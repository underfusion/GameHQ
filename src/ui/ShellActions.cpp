#include "ui/ShellActions.h"

#include <QDesktopServices>
#include <QDir>
#include <QProcess>
#include <QUrl>
#include <QDebug>

void ShellActions::openFile(const QString& filePath)
{
    if (filePath.isEmpty())
        return;
    QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));
}

void ShellActions::showInFolder(const QString& filePath)
{
    if (filePath.isEmpty())
        return;

    // Explorer recognizes the select form only without a space after /select,.
    const QString nativePath = QDir::toNativeSeparators(filePath);
    QProcess proc;
    proc.setProgram(QStringLiteral("explorer.exe"));
    proc.setNativeArguments(QStringLiteral("/select,\"%1\"").arg(nativePath));
    if (!proc.startDetached())
        qWarning() << "showInFolder: failed to launch explorer for" << nativePath;
}
