#pragma once

#include "updates/ReleaseInfo.h"

#include <QByteArray>
#include <QFile>
#include <QObject>
#include <QString>

class QNetworkAccessManager;
class QNetworkReply;
class QUrl;

// Downloads one release package into an install-local staging directory.
//
// Trust order (docs/release-manifest-security-review.md): the signed manifest
// and its detached signature are fetched first and verified over their exact
// downloaded bytes. Only then is the archive the manifest authorises fetched,
// and it is accepted only when its length and SHA-256 match the signed
// artifact record. The sibling ".sha256" asset is never a trust root.
class UpdateDownloader : public QObject
{
    Q_OBJECT
public:
    // trustStatePath must live in the user data root, outside the replaceable
    // program allowlist, so a rollback of the program files cannot lower the
    // stored release sequence.
    UpdateDownloader(QString stagingRoot, QString trustStatePath, QObject *parent = nullptr);

    void start(const ReleaseInfo &release);
    void cancel();
    bool isActive() const { return m_reply != nullptr; }

    static bool parseChecksum(const QByteArray &contents, const QString &expectedFileName,
                              QByteArray &digestOut, QString &errorOut);
    static bool verifyFile(const QString &path, const QByteArray &expectedDigest,
                           QByteArray &actualDigestOut, QString &errorOut);

Q_SIGNALS:
    void progressChanged(int percent);
    void ready(const VerifiedUpdate &verified);
    void cancelled();
    void failed(const QString &errorText);

private:
    enum class Transfer { None, Manifest, Signature, Package };

    void beginTransfer(Transfer transfer, const QUrl &url, const QString &finalPath,
                       qint64 maximumBytes, qint64 expectedBytes = 0);
    bool consumeAvailableData();
    bool publishPartial();
    void finishTransfer();
    // Verifies the downloaded manifest/signature pair and records what it
    // authorises. Returns false (after calling fail) on any rejection.
    bool acceptVerifiedManifest();
    void fail(const QString &reason);
    void clearReply();
    void removeAttemptFiles();
    static void removeStalePartials(const QString &root);

    QNetworkAccessManager *m_network;
    QString m_stagingRoot;
    QString m_trustStatePath;
    ReleaseInfo m_release;
    VerifiedUpdate m_verified;
    QNetworkReply *m_reply = nullptr;
    QFile m_output;
    Transfer m_transfer = Transfer::None;
    QString m_finalPath;
    QString m_packagePath;
    QString m_manifestPath;
    QString m_signaturePath;
    qint64 m_receivedBytes = 0;
    qint64 m_maximumBytes = 0;
    qint64 m_expectedBytes = 0;
    int m_lastProgress = -1;
    bool m_cancelRequested = false;
};
