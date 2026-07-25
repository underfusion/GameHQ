#include "updates/UpdateDownloader.h"

#include "security/ReleaseManifest.h"

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

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace
{
constexpr qint64 kMaximumPackageBytes = 2LL * 1024 * 1024 * 1024;
constexpr int kTransferTimeoutMs = 60000;

bool isHttps(const QUrl &url)
{
    return url.isValid() && url.scheme().compare(QStringLiteral("https"), Qt::CaseInsensitive) == 0;
}

std::vector<std::uint8_t> readAllBytes(const QString &path, qint64 maximumBytes, bool &okOut)
{
    okOut = false;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly) || file.size() > maximumBytes)
        return {};
    const QByteArray data = file.readAll();
    if (data.size() != file.size())
        return {};
    okOut = true;
    return {reinterpret_cast<const std::uint8_t *>(data.constData()),
            reinterpret_cast<const std::uint8_t *>(data.constData() + data.size())};
}
} // namespace

UpdateDownloader::UpdateDownloader(QString stagingRoot, QString trustStatePath, QObject *parent)
    : QObject(parent)
    , m_network(new QNetworkAccessManager(this))
    , m_stagingRoot(QDir::cleanPath(std::move(stagingRoot)))
    , m_trustStatePath(QDir::cleanPath(std::move(trustStatePath)))
{
    removeStalePartials(m_stagingRoot);
}

void UpdateDownloader::start(const ReleaseInfo &release)
{
    if (isActive())
        return;

    m_cancelRequested = false;
    m_release = release;
    m_verified = {};
    m_packagePath.clear();
    m_manifestPath.clear();
    m_signaturePath.clear();
    if (release.zipName.isEmpty() || QFileInfo(release.zipName).fileName() != release.zipName) {
        fail(QStringLiteral("The update package name is invalid."));
        return;
    }
    if (!isHttps(QUrl(release.zipUrl)) || !isHttps(QUrl(release.manifestUrl))
        || !isHttps(QUrl(release.signatureUrl))) {
        fail(QStringLiteral("The update download must use HTTPS."));
        return;
    }
    if (release.manifestUrl.isEmpty() || release.signatureUrl.isEmpty()) {
        fail(QStringLiteral("This release has no signed manifest, so it cannot be installed."));
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
    m_manifestPath = QDir(m_stagingRoot).filePath(QStringLiteral("gamehq-release.json"));
    m_signaturePath = QDir(m_stagingRoot).filePath(QStringLiteral("gamehq-release.sig"));
    removeAttemptFiles();
    qInfo() << "Update download starting: version" << release.version
            << "asset" << release.zipName << "advertised bytes" << release.zipSize;
    // Fetch the authorisation before the payload: a release whose manifest does
    // not verify must never cost the user a multi-megabyte download.
    beginTransfer(Transfer::Manifest, QUrl(release.manifestUrl), m_manifestPath,
                  static_cast<qint64>(release_manifest::kMaximumManifestBytes));
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

    if (completed == Transfer::Manifest) {
        beginTransfer(Transfer::Signature, QUrl(m_release.signatureUrl), m_signaturePath,
                      static_cast<qint64>(release_manifest::kMaximumSignatureBytes));
        return;
    }

    if (completed == Transfer::Signature) {
        if (!acceptVerifiedManifest())
            return;
        // Size and destination now come from the signed record, not from the
        // GitHub asset listing.
        beginTransfer(Transfer::Package, QUrl(m_release.zipUrl), m_packagePath,
                      m_verified.artifactSize, m_verified.artifactSize);
        return;
    }

    QString error;
    const QByteArray expectedDigest = QByteArray::fromHex(m_verified.artifactSha256.toLatin1());
    QByteArray actualDigest;
    if (expectedDigest.size() != QCryptographicHash::hashLength(QCryptographicHash::Sha256)) {
        fail(QStringLiteral("The signed manifest artifact hash is malformed."));
        return;
    }
    if (!verifyFile(m_packagePath, expectedDigest, actualDigest, error)) {
        fail(error);
        return;
    }
    if (completedBytes != m_verified.artifactSize) {
        fail(QStringLiteral("The update package length did not match the signed manifest."));
        return;
    }

    m_verified.packagePath = m_packagePath;
    m_verified.packageSha256 = actualDigest;
    if (!m_verified.isValid()) {
        fail(QStringLiteral("The verified update evidence is incomplete."));
        return;
    }

    m_transfer = Transfer::None;
    Q_EMIT progressChanged(100);
    qInfo() << "Update package verified against signed manifest: version" << m_verified.version
            << "asset" << m_verified.artifactName << "bytes" << completedBytes
            << "keyId" << m_verified.keyId << "sequence" << m_verified.releaseSequence;
    Q_EMIT ready(m_verified);
}

bool UpdateDownloader::acceptVerifiedManifest()
{
    bool ok = false;
    const std::vector<std::uint8_t> manifestBytes = readAllBytes(
        m_manifestPath, static_cast<qint64>(release_manifest::kMaximumManifestBytes), ok);
    if (!ok) {
        fail(QStringLiteral("GameHQ could not read the downloaded release manifest."));
        return false;
    }
    bool signatureOk = false;
    const std::vector<std::uint8_t> signatureBytes = readAllBytes(
        m_signaturePath, static_cast<qint64>(release_manifest::kMaximumSignatureBytes), signatureOk);
    if (!signatureOk) {
        fail(QStringLiteral("GameHQ could not read the downloaded release signature."));
        return false;
    }
    const std::string signatureText(reinterpret_cast<const char *>(signatureBytes.data()),
                                    signatureBytes.size());

    // Anti-rollback state lives in the user data root so replacing the program
    // files can never lower it.
    release_trust::SequenceState previous;
    std::string stateError;
    if (!release_trust::loadSequenceState(std::filesystem::path(m_trustStatePath.toStdWString()),
                                          previous, stateError)) {
        // Corrupt state fails closed and needs an explicit recovery rather than
        // a silent reset to zero.
        fail(QStringLiteral("GameHQ could not read its release trust state: %1")
                 .arg(QString::fromStdString(stateError)));
        return false;
    }

    release_manifest::AcceptedRelease accepted;
    std::string error;
    if (!release_manifest::verifyAndParse(manifestBytes, signatureText, &previous, accepted, error)) {
        fail(QStringLiteral("This release is not authorised by a trusted signature: %1")
                 .arg(QString::fromStdString(error)));
        return false;
    }
    if (QString::fromStdString(accepted.manifest.version) != m_release.version) {
        fail(QStringLiteral("The signed manifest describes a different version than the release."));
        return false;
    }
    const release_manifest::Artifact *update = accepted.manifest.artifactOfKind("update");
    if (!update) {
        fail(QStringLiteral("The signed manifest does not authorise an update package."));
        return false;
    }
    // Bind the GitHub asset to the signed record by exact name. A release that
    // renamed or swapped the archive can no longer be installed.
    if (QString::fromStdString(update->fileName) != m_release.zipName) {
        fail(QStringLiteral("The signed manifest names a different update package."));
        return false;
    }
    if (update->size == 0 || update->size > static_cast<std::uint64_t>(kMaximumPackageBytes)) {
        fail(QStringLiteral("The signed update package size is out of range."));
        return false;
    }

    if (!release_trust::storeSequenceStateAtomically(
            std::filesystem::path(m_trustStatePath.toStdWString()),
            {accepted.manifest.releaseSequence, accepted.manifestSha256}, stateError)) {
        fail(QStringLiteral("GameHQ could not record the release trust state: %1")
                 .arg(QString::fromStdString(stateError)));
        return false;
    }

    m_verified.version = QString::fromStdString(accepted.manifest.version);
    m_verified.manifestPath = m_manifestPath;
    m_verified.signaturePath = m_signaturePath;
    m_verified.manifestSha256 = QString::fromStdString(accepted.manifestSha256);
    m_verified.keyId = QString::fromStdString(accepted.keyId);
    m_verified.releaseSequence = accepted.manifest.releaseSequence;
    m_verified.artifactName = QString::fromStdString(update->fileName);
    m_verified.artifactSize = static_cast<qint64>(update->size);
    m_verified.artifactSha256 = QString::fromStdString(update->sha256);
    std::string canonicalSignature;
    if (!release_manifest::normalizeSignatureText(signatureText, canonicalSignature)) {
        fail(QStringLiteral("The release signature is not in its canonical form."));
        return false;
    }
    m_verified.signature = QString::fromStdString(canonicalSignature);
    qInfo() << "Release manifest verified: version" << m_verified.version
            << "keyId" << m_verified.keyId << "sequence" << m_verified.releaseSequence;
    return true;
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
                                m_manifestPath, m_manifestPath + QStringLiteral(".partial"),
                                m_signaturePath, m_signaturePath + QStringLiteral(".partial") };
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
