#include "updates/UpdatePreflight.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QStorageInfo>

namespace UpdatePreflight
{
bool check(const QString &packageRoot, qint64 downloadBytes, QString &error)
{
    error.clear();
    const QString root = QDir::cleanPath(QFileInfo(packageRoot).absoluteFilePath());
    if (root.startsWith(QStringLiteral("//")) || root.startsWith(QStringLiteral("\\\\"))) {
        error = QStringLiteral("Automatic updating is not supported from a network share. Use the release page instead.");
        return false;
    }
    if (root.size() > 180) {
        error = QStringLiteral("This installation path is too long for safe automatic replacement. Use a shorter local path.");
        return false;
    }
    const QString launcher = QDir(root).filePath(QStringLiteral("GameHQ.exe"));
    const QString app = QDir(root).filePath(QStringLiteral("app/GameHQ.exe"));
    const QString helper = QDir(root).filePath(QStringLiteral("GameHQUpdater.exe"));
    if (!QFileInfo(launcher).isFile() || !QFileInfo(app).isFile()
        || !QFileInfo(helper).isFile() || !QFileInfo(helper).isExecutable()) {
        error = QStringLiteral("Automatic updating is available only in a complete packaged GameHQ installation.");
        return false;
    }
    const QString update = QDir(root).filePath(QStringLiteral(".update"));
    if (QFileInfo::exists(QDir(update).filePath(QStringLiteral("transaction.phase")))
        || QFileInfo::exists(QDir(update).filePath(QStringLiteral("swap.manifest")))) {
        error = QStringLiteral("A previous update must finish recovery before another update can start.");
        return false;
    }
    QStorageInfo storage(root);
    if (!storage.isValid() || !storage.isReady() || storage.isReadOnly()) {
        error = QStringLiteral("The GameHQ installation is not on a writable local volume.");
        return false;
    }
    const QByteArray fs = storage.fileSystemType().toLower();
    if (fs != "ntfs" && fs != "refs") {
        error = QStringLiteral("Automatic updating currently requires NTFS or ReFS; use the release page on this drive.");
        return false;
    }
    if (!QDir().mkpath(update)) {
        error = QStringLiteral("GameHQ cannot create its update staging directory here.");
        return false;
    }
    const QString probePath = QDir(update).filePath(QStringLiteral("preflight.write-test"));
    QSaveFile probe(probePath);
    if (!probe.open(QIODevice::WriteOnly) || probe.write("ok", 2) != 2 || !probe.commit()) {
        QFile::remove(probePath);
        error = QStringLiteral("GameHQ cannot safely write update files in this installation.");
        return false;
    }
    QFile::remove(probePath);

    quint64 programBytes = 0;
    const QStringList owned = { QStringLiteral("GameHQ.exe"), QStringLiteral("GameHQUpdater.exe"),
        QStringLiteral("app"), QStringLiteral("README.txt"), QStringLiteral("LICENSE.txt"),
        QStringLiteral("THIRD_PARTY_NOTICES.md"), QStringLiteral("licenses") };
    for (const QString &name : owned) {
        const QFileInfo item(QDir(root).filePath(name));
        if (item.isFile()) {
            programBytes += static_cast<quint64>(item.size());
        } else if (item.isDir()) {
            QDirIterator files(item.absoluteFilePath(), QDir::Files | QDir::NoSymLinks,
                               QDirIterator::Subdirectories);
            while (files.hasNext()) {
                files.next();
                programBytes += static_cast<quint64>(files.fileInfo().size());
            }
        }
    }
    const quint64 zipBytes = static_cast<quint64>(qMax<qint64>(downloadBytes, 0));
    const quint64 required = zipBytes + qMax<quint64>(zipBytes * 4, 64ULL * 1024 * 1024)
        + programBytes + 64ULL * 1024 * 1024;
    if (storage.bytesAvailable() < 0
        || static_cast<quint64>(storage.bytesAvailable()) < required) {
        error = QStringLiteral("There is not enough free space for download, staging and rollback backup.");
        return false;
    }
    return true;
}
}
