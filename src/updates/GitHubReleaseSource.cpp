#include "updates/GitHubReleaseSource.h"
#include "updates/VersionNumber.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>

namespace
{
constexpr int kTransferTimeoutMs = 15000;
constexpr int kMaxAttempts = 2; // one retry on a transient network error

QString updateZipName(const QString &normalizedVersion)
{
    return QStringLiteral("GameHQ-%1-win64-update.zip").arg(normalizedVersion);
}

QString updateZipChecksumName(const QString &normalizedVersion)
{
    return updateZipName(normalizedVersion) + QStringLiteral(".sha256");
}

// GitHub reports "confirmed" rate limiting via these headers; a bare 403/429
// without them is an ordinary source error, not a rate limit, and must not
// be treated as one (a persistent false positive would make GameHQ stop
// checking for updates for users behind shared NAT).
bool isConfirmedRateLimit(const QNetworkReply *reply, qint64 &resetEpochSecondsOut)
{
    const QByteArray remaining = reply->rawHeader("x-ratelimit-remaining");
    const QByteArray reset = reply->rawHeader("x-ratelimit-reset");
    if (remaining.isEmpty() || remaining.toLongLong() != 0)
        return false;
    resetEpochSecondsOut = reset.isEmpty() ? 0 : reset.toLongLong();
    return true;
}
} // namespace

GitHubReleaseSource::GitHubReleaseSource(QString owner, QString repo, QObject *parent)
    : QObject(parent)
    , m_network(new QNetworkAccessManager(this))
    , m_owner(std::move(owner))
    , m_repo(std::move(repo))
{
}

void GitHubReleaseSource::checkLatest(const QString &ifNoneMatchEtag)
{
    sendRequest(ifNoneMatchEtag, kMaxAttempts);
}

void GitHubReleaseSource::sendRequest(const QString &ifNoneMatchEtag, int attemptsLeft)
{
    const QString url = QStringLiteral("https://api.github.com/repos/%1/%2/releases/latest")
                             .arg(m_owner, m_repo);
    QNetworkRequest request{QUrl(url)};
    request.setHeader(QNetworkRequest::UserAgentHeader,
                       QStringLiteral("GameHQ-UpdateChecker (+https://github.com/%1/%2)").arg(m_owner, m_repo));
    request.setRawHeader("Accept", "application/vnd.github+json");
    request.setRawHeader("X-GitHub-Api-Version", "2022-11-28");
    if (!ifNoneMatchEtag.isEmpty())
        request.setRawHeader("If-None-Match", ifNoneMatchEtag.toUtf8());
    request.setTransferTimeout(kTransferTimeoutMs);

    QNetworkReply *reply = m_network->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, ifNoneMatchEtag, attemptsLeft]() {
        reply->deleteLater();
        handleReply(reply, ifNoneMatchEtag, attemptsLeft);
    });
}

void GitHubReleaseSource::handleReply(QNetworkReply *reply, const QString &ifNoneMatchEtag, int attemptsLeft)
{
    const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    qint64 resetEpoch = 0;
    if ((httpStatus == 403 || httpStatus == 429) && isConfirmedRateLimit(reply, resetEpoch)) {
        Q_EMIT rateLimited(resetEpoch);
        return;
    }

    if (httpStatus == 304) {
        Q_EMIT unchanged(ifNoneMatchEtag);
        return;
    }

    if (httpStatus == 404) {
        Q_EMIT notFound();
        return;
    }

    if (reply->error() != QNetworkReply::NoError || httpStatus != 200) {
        // Transient (no confirmed HTTP status yet, e.g. timeout/DNS failure):
        // retry once before giving up.
        const bool transient = httpStatus == 0;
        if (transient && attemptsLeft > 1) {
            QTimer::singleShot(1000, this, [this, ifNoneMatchEtag, attemptsLeft]() {
                sendRequest(ifNoneMatchEtag, attemptsLeft - 1);
            });
            return;
        }
        Q_EMIT failed(reply->errorString());
        return;
    }

    const QByteArray body = reply->readAll();
    if (body.size() > 4 * 1024 * 1024) {
        Q_EMIT failed(QStringLiteral("release response is unreasonably large"));
        return;
    }
    const QJsonDocument doc = QJsonDocument::fromJson(body);
    if (!doc.isObject()) {
        Q_EMIT failed(QStringLiteral("malformed release JSON"));
        return;
    }
    const QJsonObject obj = doc.object();

    const QString tagName = obj.value(QStringLiteral("tag_name")).toString();
    const auto version = VersionNumber::parse(tagName);
    if (!version.has_value()) {
        Q_EMIT failed(QStringLiteral("release tag \"%1\" is not a valid version").arg(tagName));
        return;
    }
    const QString normalizedVersion = version->toString();

    ReleaseInfo info;
    info.version = normalizedVersion;
    info.name = obj.value(QStringLiteral("name")).toString();
    info.notes = obj.value(QStringLiteral("body")).toString();
    info.publishedAt = QDateTime::fromString(obj.value(QStringLiteral("published_at")).toString(), Qt::ISODate);
    info.webUrl = obj.value(QStringLiteral("html_url")).toString();
    info.prerelease = obj.value(QStringLiteral("prerelease")).toBool();
    info.draft = obj.value(QStringLiteral("draft")).toBool();

    const QString expectedZip = updateZipName(normalizedVersion);
    const QString expectedChecksum = updateZipChecksumName(normalizedVersion);
    const QJsonArray assets = obj.value(QStringLiteral("assets")).toArray();
    for (const QJsonValue &assetValue : assets) {
        const QJsonObject asset = assetValue.toObject();
        const QString name = asset.value(QStringLiteral("name")).toString();
        if (name == expectedZip) {
            info.zipName = name;
            info.zipUrl = asset.value(QStringLiteral("browser_download_url")).toString();
            info.zipSize = static_cast<qint64>(asset.value(QStringLiteral("size")).toDouble());
        } else if (name == expectedChecksum) {
            info.checksumUrl = asset.value(QStringLiteral("browser_download_url")).toString();
        }
    }

    const QString etag = QString::fromUtf8(reply->rawHeader("ETag"));
    Q_EMIT succeeded(info, etag);
}
