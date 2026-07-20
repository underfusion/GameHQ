#include "updates/UpdateService.h"
#include "updates/GitHubReleaseSource.h"
#include "updates/UpdateDownloader.h"
#include "updates/VersionNumber.h"
#include "updates/UpdatePreflight.h"

#include <QDesktopServices>
#include <QDir>
#include <QLocale>
#include <QTimeZone>
#include <QUrl>
#include <QtLogging>

UpdateService::UpdateService(QString owner, QString repo, QString installedVersion,
                             QString stagingRoot, QObject *parent)
    : QObject(parent)
    , m_source(new GitHubReleaseSource(std::move(owner), std::move(repo), this))
    , m_downloader(new UpdateDownloader(stagingRoot, this))
    , m_installedVersion(std::move(installedVersion))
    , m_packageRoot(QDir(stagingRoot).absoluteFilePath(QStringLiteral("../..")))
{
    connect(m_source, &GitHubReleaseSource::succeeded, this, &UpdateService::onSucceeded);
    connect(m_source, &GitHubReleaseSource::unchanged, this, &UpdateService::onUnchanged);
    connect(m_source, &GitHubReleaseSource::notFound, this, &UpdateService::onNotFound);
    connect(m_source, &GitHubReleaseSource::rateLimited, this, &UpdateService::onRateLimited);
    connect(m_source, &GitHubReleaseSource::failed, this, &UpdateService::onFailed);
    connect(m_downloader, &UpdateDownloader::progressChanged, this, [this](int progress) {
        if (m_progress == progress)
            return;
        m_progress = progress;
        Q_EMIT progressChanged();
    });
    connect(m_downloader, &UpdateDownloader::ready, this,
            [this](const QString &packagePath, const QByteArray &sha256) {
        m_downloadedPackagePath = packagePath;
        m_downloadedSha256 = sha256;
        setState(State::ReadyToInstall);
    });
    connect(m_downloader, &UpdateDownloader::cancelled, this, [this] {
        m_progress = 0;
        Q_EMIT progressChanged();
        setState(State::UpdateAvailable);
    });
    connect(m_downloader, &UpdateDownloader::failed, this, [this](const QString &reason) {
        m_errorText = reason;
        Q_EMIT errorChanged();
        m_progress = 0;
        Q_EMIT progressChanged();
        m_failedDuringCheck = false;
        setState(State::Failed);
    });
}

QString UpdateService::stateName() const
{
    switch (m_state) {
    case State::Idle: return QStringLiteral("Idle");
    case State::Checking: return QStringLiteral("Checking");
    case State::UpToDate: return QStringLiteral("UpToDate");
    case State::UpdateAvailable: return QStringLiteral("UpdateAvailable");
    case State::Downloading: return QStringLiteral("Downloading");
    case State::ReadyToInstall: return QStringLiteral("ReadyToInstall");
    case State::PreparingForUpdate: return QStringLiteral("PreparingForUpdate");
    case State::Quiescent: return QStringLiteral("Quiescent");
    case State::Installing: return QStringLiteral("Installing");
    case State::Failed: return QStringLiteral("Failed");
    }
    return QStringLiteral("Idle");
}

void UpdateService::setState(State state)
{
    if (m_state == state)
        return;
    m_state = state;
    Q_EMIT stateChanged();
}

bool UpdateService::releaseIsNewerThanInstalled(const ReleaseInfo &release) const
{
    const auto latest = VersionNumber::parse(release.version);
    const auto installed = VersionNumber::parse(m_installedVersion);
    return latest.has_value() && installed.has_value() && *latest > *installed;
}

// A check that produced nothing usable never disturbs the last known-good
// result; fall back to whatever that result still justifies.
UpdateService::State UpdateService::fallbackAfterCheck() const
{
    return m_release.has_value() && releaseIsNewerThanInstalled(*m_release)
            && m_release->version != m_skippedVersion
        ? State::UpdateAvailable
        : State::UpToDate;
}

void UpdateService::checkNow()
{
    if (m_state == State::Checking)
        return;
    // Rapid re-clicks of "Check again" must not turn into request bursts
    // against GitHub's anonymous rate limit.
    if (m_lastCheckRequest.isValid() && m_lastCheckRequest.elapsed() < 5000)
        return;
    m_lastCheckRequest.start();
    m_errorText.clear();
    Q_EMIT errorChanged();
    setState(State::Checking);
    m_source->checkLatest(m_etag);
}

void UpdateService::applyRelease(const ReleaseInfo &release)
{
    if (release.draft) {
        qInfo() << "UpdateService: ignoring draft release" << release.version;
        setState(State::UpToDate);
        return;
    }
    if (release.prerelease) {
        qInfo() << "UpdateService: ignoring prerelease" << release.version;
        setState(State::UpToDate);
        return;
    }
    if (!releaseIsNewerThanInstalled(release)) {
        qInfo() << "UpdateService: latest release" << release.version
                 << "is not newer than installed" << m_installedVersion;
        setState(State::UpToDate);
        return;
    }
    if (release.version == m_skippedVersion) {
        qInfo() << "UpdateService: release" << release.version << "was skipped by the user";
        m_release = release;
        Q_EMIT releaseChanged();
        setState(State::UpToDate);
        return;
    }

    m_release = release;
    Q_EMIT releaseChanged();
    setState(State::UpdateAvailable);
}

void UpdateService::onSucceeded(const ReleaseInfo &release, const QString &etag)
{
    // A newer release that is still uploading its assets is not a completed
    // check: skip the ETag (a cached one would answer 304 forever and hide the
    // finished assets) and leave lastChecked stale so the hourly automatic
    // wake retries instead of waiting out the 24h gate.
    const bool offerable = !release.draft && !release.prerelease
        && releaseIsNewerThanInstalled(release);
    const bool incomplete = offerable && !release.hasCompleteUpdateAssets();
    if (!incomplete) {
        m_lastChecked = QDateTime::currentDateTimeUtc();
        Q_EMIT lastCheckedChanged();
        if (!etag.isEmpty() && etag != m_etag) {
            m_etag = etag;
            Q_EMIT etagUpdated(etag);
        }
    }
    if (m_revalidatingInstall) {
        m_revalidatingInstall = false;
        const bool unchanged = m_release.has_value() && !release.draft && !release.prerelease
            && release.version == m_release->version
            && release.zipName == m_release->zipName
            && release.zipUrl == m_release->zipUrl
            && release.checksumUrl == m_release->checksumUrl
            && release.zipSize == m_release->zipSize;
        if (!unchanged) {
            m_errorText = QStringLiteral("The release changed after download. Check again before installing.");
            Q_EMIT errorChanged();
            if (incomplete) {
                setState(fallbackAfterCheck());
                return;
            }
            applyRelease(release);
            return;
        }
        Q_EMIT installApproved(release.version, m_downloadedPackagePath,
                               m_downloadedSha256);
        return;
    }
    if (incomplete) {
        qInfo() << "UpdateService: release" << release.version
                << "is missing its update assets; keeping the previous result and re-checking later";
        setState(fallbackAfterCheck());
        return;
    }
    applyRelease(release);
}

void UpdateService::onUnchanged(const QString & /*etag*/)
{
    if (m_revalidatingInstall) {
        m_revalidatingInstall = false;
        cancelPreparation(QStringLiteral("The release could not be freshly revalidated before installation."));
        return;
    }
    m_lastChecked = QDateTime::currentDateTimeUtc();
    Q_EMIT lastCheckedChanged();
    setState(fallbackAfterCheck());
}

void UpdateService::onNotFound()
{
    if (m_revalidatingInstall) {
        m_revalidatingInstall = false;
        cancelPreparation(QStringLiteral("The downloaded release was withdrawn before installation."));
        return;
    }
    m_lastChecked = QDateTime::currentDateTimeUtc();
    Q_EMIT lastCheckedChanged();
    m_errorText.clear();
    Q_EMIT errorChanged();
    setState(State::UpToDate);
}

void UpdateService::onRateLimited(qint64 resetEpochSeconds)
{
    const QDateTime resetAt = resetEpochSeconds > 0
        ? QDateTime::fromSecsSinceEpoch(resetEpochSeconds, QTimeZone::UTC)
        : QDateTime();
    if (m_revalidatingInstall) {
        m_revalidatingInstall = false;
        cancelPreparation(QStringLiteral("GitHub could not revalidate this release before installation. Try again later."));
        return;
    }
    qWarning() << "UpdateService: GitHub rate-limited the update check, resets at" << resetAt;
    m_errorText = resetAt.isValid()
        ? QStringLiteral("GitHub temporarily limited update checks. GameHQ will try again after %1.")
              .arg(QLocale().toString(resetAt.toLocalTime(), QLocale::ShortFormat))
        : QStringLiteral("GitHub temporarily limited update checks. GameHQ will try again later.");
    Q_EMIT errorChanged();
    // Never treat rate limiting as "no update" or as a hard failure: fall back
    // to whatever the last known-good result was.
    setState(fallbackAfterCheck());
}

void UpdateService::onFailed(const QString &errorText)
{
    if (m_revalidatingInstall) {
        m_revalidatingInstall = false;
        cancelPreparation(QStringLiteral("The release could not be revalidated: %1").arg(errorText));
        return;
    }
    qWarning() << "UpdateService: check failed:" << errorText;
    m_errorText = errorText;
    Q_EMIT errorChanged();
    // A failed check never disturbs a known-good result; only report Failed
    // when there is nothing good to fall back to.
    if (m_release.has_value()) {
        setState(fallbackAfterCheck());
    } else {
        m_failedDuringCheck = true;
        setState(State::Failed);
    }
}

void UpdateService::downloadUpdate()
{
    if (!m_release.has_value() || m_downloader->isActive())
        return;
    QString preflightError;
    if (!UpdatePreflight::check(m_packageRoot, m_release->zipSize, preflightError)) {
        m_errorText = preflightError;
        Q_EMIT errorChanged();
        m_failedDuringCheck = false;
        setState(State::Failed);
        return;
    }
    m_errorText.clear();
    Q_EMIT errorChanged();
    m_progress = 0;
    Q_EMIT progressChanged();
    setState(State::Downloading);
    m_downloader->start(*m_release);
}

void UpdateService::cancelDownload()
{
    m_downloader->cancel();
}

void UpdateService::installAndRestart()
{
    if (m_state != State::ReadyToInstall)
        return;
    QString preflightError;
    if (!UpdatePreflight::check(m_packageRoot, m_release ? m_release->zipSize : 0,
                                preflightError)) {
        m_errorText = preflightError;
        Q_EMIT errorChanged();
        m_failedDuringCheck = false;
        setState(State::Failed);
        return;
    }
    m_errorText.clear();
    Q_EMIT errorChanged();
    setState(State::PreparingForUpdate);
    Q_EMIT prepareForUpdateRequested();
}

void UpdateService::markQuiescent()
{
    if (m_state != State::PreparingForUpdate)
        return;
    setState(State::Quiescent);
    Q_EMIT quiescenceReached();
    m_revalidatingInstall = true;
    m_source->checkLatest(QString());
}

void UpdateService::markInstalling()
{
    if (m_state == State::Quiescent)
        setState(State::Installing);
}

void UpdateService::cancelPreparation(const QString &reason)
{
    if (m_state != State::PreparingForUpdate && m_state != State::Quiescent)
        return;
    m_errorText = reason;
    Q_EMIT errorChanged();
    setState(State::ReadyToInstall);
}

void UpdateService::skipVersion()
{
    if (!m_release.has_value())
        return;
    m_skippedVersion = m_release->version;
    Q_EMIT versionSkipped(m_skippedVersion);
    setState(State::UpToDate);
}

void UpdateService::openReleasePage() const
{
    const QString url = m_release ? m_release->webUrl : QString();
    if (!url.isEmpty())
        QDesktopServices::openUrl(QUrl(url));
}
