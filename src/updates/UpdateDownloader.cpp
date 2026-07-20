#include "updates/UpdateDownloader.h"

#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QUrl>
#include <QtLogging>

namespace
{
constexpr qint64 kMaximumPackageBytes = 2LL * 1024 * 1024 * 1024;
constexpr qint64 kMaximumChecksumBytes = 4096;
constexpr int kTransferTimeoutMs = 60000;

bool isHttps(const QUrl &url)
{
    return url.isValid() && url.scheme().compare(QStringLiteral("https"), Qt::CaseInsensitive) == 0;
}
} // namespace

UpdateDownloader::UpdateDownloader(QString stagingRoot, QObject *parent)
    : QObject(parent)
    , m_network(new QNetworkAccessManager(this))
    , m_stagingRoot(QDir::cleanPath(std::move(stagingRoot)))
{
    removeStalePartials(m_stagingRoot);
}

void UpdateDownloader::start(const ReleaseInfo &release)
{
    if (isActive())
        return;

    m_cancelRequested = false;
    m_release = release;
    m_packagePath.clear();
    m_checksumPath.clear();
    if (release.zipName.isEmpty() || QFileInfo(release.zipName).fileName() != release.zipName) {
        fail(QStringLiteral("The update package name is invalid."));
        return;
    }
    if (!isHttps(QUrl(release.zipUrl)) || !isHttps(QUrl(release.checksumUrl))) {
        fail(QStringLiteral("The update download must use HTTPS."));
        return;
    }
    if (release.zipSize <= 0 || release.zipSize > kMaximumPackageBytes) {
        fail(QStringLiteral("The update package size is missing or exceeds the safety limit."));
        return;
    }
    if (!QDir().mkpath(m_stagingRoot)) {
        fail(QStringLiteral("GameHQ could not create the update staging directory."));
        return;
    }

    m_packagePath = QDir(m_stagingRoot).filePath(release.zipName);
    m_checksumPath = m_packagePath + QStringLiteral(".sha256");
    removeAttemptFiles();
    qInfo() << "Update download starting: version" << release.version
            << "asset" << release.zipName << "expected bytes" << release.zipSize;
    beginTransfer(Transfer::Package, QUrl(release.zipUrl), m_packagePath,
                  kMaximumPackageBytes, release.zipSize);
}

void UpdateDownloader::cancel()
{
    if (!isActive())
        return;
    m_cancelRequested = true;
    clearReply();
    removeAttemptFiles();
    m_transfer = Transfer::None;
    qInfo() << "Update download cancelled: version" << m_release.version
            << "asset" << m_release.zipName << "received bytes" << m_receivedBytes;
    Q_EMIT cancelled();
}

void UpdateDownloader::beginTransfer(Transfer transfer, const QUrl &url,
                                     const QString &finalPath, qint64 maximumBytes,
                                     qint64 expectedBytes)
{
    m_transfer = transfer;
    m_finalPath = finalPath;
    m_receivedBytes = 0;
    m_maximumBytes = maximumBytes;
    m_expectedBytes = expectedBytes;
    m_lastProgress = -1;

    m_output.setFileName(finalPath + QStringLiteral(".partial"));
    if (!m_output.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        fail(QStringLiteral("GameHQ could not create the partial update file."));
        return;
    }

    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setTransferTimeout(kTransferTimeoutMs);
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QStringLiteral("GameHQ-Updater/%1").arg(m_release.version));
    m_reply = m_network->get(request);
    connect(m_reply, &QNetworkReply::readyRead, this, [this] { consumeAvailableData(); });
    connect(m_reply, &QNetworkReply::downloadProgress, this,
            [this](qint64 received, qint64 total) {
        if (m_transfer != Transfer::Package)
            return;
        const qint64 denominator = m_expectedBytes > 0 ? m_expectedBytes : total;
        if (denominator <= 0)
            return;
        const int percent = qBound(0, static_cast<int>((received * 100) / denominator), 100);
        if (percent != m_lastProgress) {
            m_lastProgress = percent;
            Q_EMIT progressChanged(percent);
        }
    });
    connect(m_reply, &QNetworkReply::finished, this, &UpdateDownloader::finishTransfer);
}

bool UpdateDownloader::consumeAvailableData()
{
    if (!m_reply)
        return false;
    const QByteArray chunk = m_reply->readAll();
    if (chunk.isEmpty())
        return true;
    if (m_receivedBytes > m_maximumBytes - chunk.size()) {
        fail(QStringLiteral("The update download exceeded its safety size limit."));
        return false;
    }
    if (m_output.write(chunk) != chunk.size()) {
        fail(QStringLiteral("GameHQ could not write the update download to disk."));
        return false;
    }
    m_receivedBytes += chunk.size();
    return true;
}

bool UpdateDownloader::publishPartial()
{
    if (!m_output.flush()) {
        fail(QStringLiteral("GameHQ could not flush the update download to disk."));
        return false;
    }
    m_output.close();
    QFile::remove(m_finalPath);
    if (!QFile::rename(m_finalPath + QStringLiteral(".partial"), m_finalPath)) {
        fail(QStringLiteral("GameHQ could not publish the completed update download."));
        return false;
    }
    return true;
}

void UpdateDownloader::finishTransfer()
{
    if (!m_reply || m_cancelRequested)
        return;
    QNetworkReply *reply = m_reply;
    if (!consumeAvailableData())
        return;
    if (!isHttps(reply->url())) {
        fail(QStringLiteral("The update download redirected away from HTTPS."));
        return;
    }
    if (reply->error() != QNetworkReply::NoError) {
        fail(QStringLiteral("Update download failed: %1").arg(reply->errorString()));
        return;
    }
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (status < 200 || status >= 300) {
        fail(QStringLiteral("Update download failed with HTTP status %1.").arg(status));
        return;
    }
    if (m_expectedBytes > 0 && m_receivedBytes != m_expectedBytes) {
        fail(QStringLiteral("The update download size did not match the release metadata."));
        return;
    }

    const Transfer completed = m_transfer;
    const qint64 completedBytes = m_receivedBytes;
    disconnect(m_reply, nullptr, this, nullptr);
    m_reply->deleteLater();
    m_reply = nullptr;
    if (!publishPartial())
        return;

    if (completed == Transfer::Package) {
        qInfo() << "Update package downloaded: version" << m_release.version
                << "asset" << m_release.zipName << "bytes" << completedBytes;
        beginTransfer(Transfer::Checksum, QUrl(m_release.checksumUrl), m_checksumPath,
                      kMaximumChecksumBytes);
        return;
    }

    QFile checksumFile(m_checksumPath);
    if (!checksumFile.open(QIODevice::ReadOnly)) {
        fail(QStringLiteral("GameHQ could not read the downloaded checksum."));
        return;
    }
    QByteArray expectedDigest;
    QString error;
    if (!parseChecksum(checksumFile.readAll(), m_release.zipName, expectedDigest, error)) {
        fail(error);
        return;
    }
    QByteArray actualDigest;
    if (!verifyFile(m_packagePath, expectedDigest, actualDigest, error)) {
        fail(error);
        return;
    }

    m_transfer = Transfer::None;
    Q_EMIT progressChanged(100);
    qInfo() << "Update package verified: version" << m_release.version
            << "asset" << m_release.zipName << "bytes" << QFileInfo(m_packagePath).size()
            << "sha256" << actualDigest.toHex();
    Q_EMIT ready(m_packagePath, actualDigest);
}

bool UpdateDownloader::parseChecksum(const QByteArray &contents, const QString &expectedFileName,
                                     QByteArray &digestOut, QString &errorOut)
{
    QList<QByteArray> lines;
    for (const QByteArray &rawLine : contents.split('\n')) {
        const QByteArray line = rawLine.trimmed();
        if (!line.isEmpty())
            lines.push_back(line);
    }
    if (lines.size() != 1) {
        errorOut = QStringLiteral("The update checksum file must contain exactly one entry.");
        return false;
    }

    static const QRegularExpression pattern(
        QStringLiteral("^([0-9A-Fa-f]{64})(?:[\\t ]+\\*?(.+))?$"));
    const QRegularExpressionMatch match = pattern.match(QString::fromLatin1(lines.front()));
    if (!match.hasMatch()) {
        errorOut = QStringLiteral("The update checksum file has an invalid format.");
        return false;
    }
    const QString namedFile = match.captured(2).trimmed();
    if (!namedFile.isEmpty() && namedFile != expectedFileName) {
        errorOut = QStringLiteral("The update checksum names a different package.");
        return false;
    }
    digestOut = QByteArray::fromHex(match.captured(1).toLatin1());
    return digestOut.size() == QCryptographicHash::hashLength(QCryptographicHash::Sha256);
}

bool UpdateDownloader::verifyFile(const QString &path, const QByteArray &expectedDigest,
                                  QByteArray &actualDigestOut, QString &errorOut)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        errorOut = QStringLiteral("GameHQ could not read the downloaded update package.");
        return false;
    }
    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (!hash.addData(&file)) {
        errorOut = QStringLiteral("GameHQ could not calculate the update checksum.");
        return false;
    }
    actualDigestOut = hash.result();
    if (actualDigestOut != expectedDigest) {
        errorOut = QStringLiteral("The update package failed SHA-256 verification and was rejected.");
        return false;
    }
    return true;
}

void UpdateDownloader::fail(const QString &reason)
{
    const qint64 received = m_receivedBytes;
    clearReply();
    removeAttemptFiles();
    m_transfer = Transfer::None;
    qWarning() << "Update download rejected: version" << m_release.version
               << "asset" << m_release.zipName << "received bytes" << received
               << "reason" << reason;
    Q_EMIT failed(reason);
}

void UpdateDownloader::clearReply()
{
    if (m_reply) {
        disconnect(m_reply, nullptr, this, nullptr);
        if (!m_reply->isFinished())
            m_reply->abort();
        m_reply->deleteLater();
        m_reply = nullptr;
    }
    if (m_output.isOpen())
        m_output.close();
}

void UpdateDownloader::removeAttemptFiles()
{
    const QStringList paths = { m_packagePath, m_packagePath + QStringLiteral(".partial"),
                                m_checksumPath, m_checksumPath + QStringLiteral(".partial") };
    for (const QString &path : paths) {
        if (!path.isEmpty())
            QFile::remove(path);
    }
}

void UpdateDownloader::removeStalePartials(const QString &root)
{
    if (!QFileInfo::exists(root))
        return;
    QDirIterator it(root, { QStringLiteral("*.partial") }, QDir::Files,
                    QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString stalePath = it.next();
        if (QFile::remove(stalePath))
            qInfo() << "Removed stale update partial" << stalePath;
        else
            qWarning() << "Could not remove stale update partial" << stalePath;
    }
}
