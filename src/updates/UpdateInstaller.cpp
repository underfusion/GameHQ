#include "updates/UpdateInstaller.h"
#include "updates/UpdateDownloader.h"
#include "core/UpdaterHandshake.h"

#include <QCoreApplication>
#include <QDir>
#include <QCryptographicHash>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QSaveFile>
#include <QRegularExpression>

#include <windows.h>

namespace UpdateInstaller
{
bool prepareTransaction(const QString &packageRoot, const QString &dataDir,
                        const QString &packagePath, const QString &version,
                        const QByteArray &sha256, QString &transactionPath,
                        QString &error)
{
    error.clear();
    const QString root = QDir::cleanPath(QFileInfo(packageRoot).absoluteFilePath());
    const QString package = QDir::cleanPath(QFileInfo(packagePath).absoluteFilePath());
    const QString downloads = QDir(root).filePath(QStringLiteral(".update/downloads"));
    const QString relative = QDir(downloads).relativeFilePath(package);
    if (relative == QStringLiteral("..") || relative.startsWith(QStringLiteral("../"))) {
        error = QStringLiteral("The verified update package is outside GameHQ's staging directory.");
        return false;
    }
    if (!QRegularExpression(QStringLiteral(R"(^\d+\.\d+\.\d+$)")).match(version).hasMatch()
        || sha256.size() != QCryptographicHash::hashLength(QCryptographicHash::Sha256)) {
        error = QStringLiteral("The verified update metadata is invalid.");
        return false;
    }
    QByteArray actual;
    QString verifyError;
    if (!UpdateDownloader::verifyFile(package, sha256, actual, verifyError)) {
        error = QStringLiteral("The update package changed before installation: %1").arg(verifyError);
        return false;
    }
    const QString update = QDir(root).filePath(QStringLiteral(".update"));
    if (!QDir().mkpath(update)) {
        error = QStringLiteral("GameHQ could not create the update transaction directory.");
        return false;
    }
    transactionPath = QDir(update).filePath(QStringLiteral("transaction.json"));
    const QJsonObject object {
        { QStringLiteral("schemaVersion"), 1 },
        { QStringLiteral("productId"), QStringLiteral("underfusion.gamehq") },
        { QStringLiteral("expectedVersion"), version },
        { QStringLiteral("expectedSha256"), QString::fromLatin1(sha256.toHex()) },
        { QStringLiteral("packageRoot"), QDir::toNativeSeparators(root) },
        { QStringLiteral("packagePath"), QDir::toNativeSeparators(package) },
        { QStringLiteral("stagingDir"), QDir::toNativeSeparators(QDir(update).filePath(QStringLiteral("staging"))) },
        { QStringLiteral("backupDir"), QDir::toNativeSeparators(QDir(update).filePath(QStringLiteral("backup"))) },
        { QStringLiteral("restartExecutable"), QDir::toNativeSeparators(QDir(root).filePath(QStringLiteral("GameHQ.exe"))) },
        { QStringLiteral("healthTokenPath"), QDir::toNativeSeparators(QDir(update).filePath(QStringLiteral("healthy.token"))) },
        { QStringLiteral("dataDir"), QDir::toNativeSeparators(QFileInfo(dataDir).absoluteFilePath()) },
        { QStringLiteral("dataSnapshotDir"), QDir::toNativeSeparators(QDir(update).filePath(QStringLiteral("data-snapshot"))) },
        { QStringLiteral("callerPid"), static_cast<qint64>(QCoreApplication::applicationPid()) },
        { QStringLiteral("phase"), QStringLiteral("download_verified") }
    };
    QSaveFile output(transactionPath);
    const QByteArray json = QJsonDocument(object).toJson(QJsonDocument::Compact);
    if (!output.open(QIODevice::WriteOnly) || output.write(json) != json.size() || !output.commit()) {
        error = QStringLiteral("GameHQ could not publish the update transaction.");
        return false;
    }
    return true;
}

bool launchPrepared(const QString &packageRoot, const QString &transactionPath,
                    QString &error)
{
    const QString root = QFileInfo(packageRoot).absoluteFilePath();
    const QString helper = QDir(root).filePath(QStringLiteral("GameHQUpdater.exe"));
    if (!QFileInfo(helper).isFile() || !QFileInfo(helper).isExecutable()) {
        error = QStringLiteral("GameHQUpdater.exe is missing or cannot run.");
        return false;
    }
    // Create the READY event before the helper starts so its SetEvent can
    // never race ahead of us; only quit the app once the helper has validated
    // the transaction (docs/updater.md "Handoff").
    const std::wstring readyName =
        handshake::readyEventNameFor(transactionPath.toStdWString());
    HANDLE ready = CreateEventW(nullptr, TRUE, FALSE, readyName.c_str());
    if (!ready) {
        error = QStringLiteral("GameHQ could not prepare the updater handshake.");
        return false;
    }
    qint64 processId = 0;
    if (!QProcess::startDetached(helper,
                                 { QStringLiteral("--apply"), transactionPath },
                                 root, &processId) || processId <= 0) {
        CloseHandle(ready);
        error = QStringLiteral("GameHQ could not start the updater helper.");
        return false;
    }
    HANDLE helperProcess = OpenProcess(SYNCHRONIZE, FALSE, static_cast<DWORD>(processId));
    HANDLE waitees[2] = { ready, helperProcess };
    const DWORD count = helperProcess ? 2 : 1;
    const DWORD result = WaitForMultipleObjects(count, waitees, FALSE, 15000);
    if (helperProcess)
        CloseHandle(helperProcess);
    CloseHandle(ready);
    if (result == WAIT_OBJECT_0)
        return true;
    error = result == WAIT_OBJECT_0 + 1
        ? QStringLiteral("The updater helper rejected the update before it became ready.")
        : QStringLiteral("The updater helper did not confirm it is ready in time.");
    return false;
}
}
