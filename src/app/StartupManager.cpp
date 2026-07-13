#include "app/StartupManager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QStringList>
#include <QDebug>

namespace
{
const QString kRunKey = QStringLiteral(
    "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run");
const QString kValueName = QStringLiteral("GameHQ");
const QStringList kLegacyValueNames = {QStringLiteral("SavePlay"), QStringLiteral("PlayHQ")};
}

QString StartupManager::executablePath()
{
    const QFileInfo realExecutable(QCoreApplication::applicationFilePath());
    const QDir realDir = realExecutable.absoluteDir();
    const QDir packageRoot(realDir.absoluteFilePath(QStringLiteral("..")));
    const QString launcher = packageRoot.absoluteFilePath(QStringLiteral("GameHQ.exe"));

    if (QFileInfo::exists(packageRoot.absoluteFilePath(QStringLiteral("portable.flag")))
        && QFileInfo::exists(launcher)) {
        return QDir::cleanPath(launcher);
    }
    return QDir::cleanPath(realExecutable.absoluteFilePath());
}

bool StartupManager::setEnabled(bool enabled) const
{
    QSettings runKey(kRunKey, QSettings::NativeFormat);
    if (enabled) {
        const QString command = QStringLiteral("\"%1\"")
            .arg(QDir::toNativeSeparators(executablePath()));
        runKey.setValue(kValueName, command);
        for (const QString& legacyName : kLegacyValueNames)
            runKey.remove(legacyName);
    } else {
        runKey.remove(kValueName);
        for (const QString& legacyName : kLegacyValueNames)
            runKey.remove(legacyName);
    }
    runKey.sync();
    if (runKey.status() != QSettings::NoError) {
        qWarning() << "Startup: could not" << (enabled ? "register" : "remove")
                   << "the per-user Run entry";
        return false;
    }
    qInfo() << "Startup:" << (enabled ? "enabled" : "disabled");
    return true;
}
