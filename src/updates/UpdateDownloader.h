#pragma once

#include "updates/ReleaseInfo.h"

#include <QByteArray>
#include <QFile>
#include <QObject>
#include <QString>

class QNetworkAccessManager;
class QNetworkReply;
class QUrl;

// Downloads one release package and its checksum into an install-local staging
// directory. Files are never published without a complete HTTPS transfer, and
// the package is never reported ready until its local SHA-256 matches.
class UpdateDownloader : public QObject
{
    Q_OBJECT
public:
    explicit UpdateDownloader(QString stagingRoot, QObject *parent = nullptr);

    void start(const ReleaseInfo &release);
    void cancel();
    bool isActive() const { return m_reply != nullptr; }

    static bool parseChecksum(const QByteArray &contents, const QString &expectedFileName,
                              QByteArray &digestOut, QString &errorOut);
    static bool verifyFile(const QString &path, const QByteArray &expectedDigest,
                           QByteArray &actualDigestOut, QString &errorOut);

Q_SIGNALS:
    void progressChanged(int percent);
    void ready(const QString &packagePath, const QByteArray &sha256);
    void cancelled();
    void failed(const QString &errorText);

private:
    enum class Transfer { None, Package, Checksum };

    void beginTransfer(Transfer transfer, const QUrl &url, const QString &finalPath,
                       qint64 maximumBytes, qint64 expectedBytes = 0);
    bool consumeAvailableData();
    bool publishPartial();
    void finishTransfer();
    void fail(const QString &reason);
    void clearReply();
    void removeAttemptFiles();
    static void removeStalePartials(const QString &root);

    QNetworkAccessManager *m_network;
    QString m_stagingRoot;
    ReleaseInfo m_release;
    QNetworkReply *m_reply = nullptr;
    QFile m_output;
    Transfer m_transfer = Transfer::None;
    QString m_finalPath;
    QString m_packagePath;
    QString m_checksumPath;
    qint64 m_receivedBytes = 0;
    qint64 m_maximumBytes = 0;
    qint64 m_expectedBytes = 0;
    int m_lastProgress = -1;
    bool m_cancelRequested = false;
};
