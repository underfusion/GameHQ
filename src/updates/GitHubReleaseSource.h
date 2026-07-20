#pragma once

#include "updates/ReleaseInfo.h"

#include <QObject>
#include <QString>

class QNetworkAccessManager;
class QNetworkReply;

// Talks to the GitHub REST API to find the current stable release for one
// repo. Read-only, no embedded token: subject to GitHub's 60 requests/hour
// per-IP anonymous limit, so callers must cache the ETag and never poll
// tightly. Emits exactly one of the signals below per checkLatest() call.
class GitHubReleaseSource : public QObject
{
    Q_OBJECT
public:
    // owner/repo identify "https://github.com/<owner>/<repo>".
    GitHubReleaseSource(QString owner, QString repo, QObject *parent = nullptr);

    // Pass back the ETag from a previous succeeded() to allow a cheap
    // "304 Not Modified" short-circuit via unchanged().
    void checkLatest(const QString &ifNoneMatchEtag = QString());

Q_SIGNALS:
    void succeeded(const ReleaseInfo &release, const QString &etag);
    void unchanged(const QString &etag);
    void notFound();
    // resetEpochSeconds is the Unix time (from x-ratelimit-reset) after which
    // a retry may succeed; 0 if GitHub did not report one.
    void rateLimited(qint64 resetEpochSeconds);
    void failed(const QString &errorText);

private:
    void sendRequest(const QString &ifNoneMatchEtag, int attemptsLeft);
    void handleReply(QNetworkReply *reply, const QString &ifNoneMatchEtag, int attemptsLeft);

    QNetworkAccessManager *m_network;
    QString m_owner;
    QString m_repo;
};
