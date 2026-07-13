#include "storage/GameIconCache.h"

#include "config/Paths.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFileIconProvider>
#include <QFileInfo>
#include <QIcon>
#include <QPixmap>

QString GameIconCache::iconPathForExecutable(const QString& executablePath)
{
    if (executablePath.isEmpty() || !QFileInfo::exists(executablePath))
        return {};

    QDir().mkpath(Paths::gameIconsDir());
    const QByteArray hash = QCryptographicHash::hash(executablePath.toUtf8(),
                                                     QCryptographicHash::Sha1).toHex();
    const QString outPath = Paths::gameIconsDir() + QLatin1Char('/')
                            + QString::fromLatin1(hash) + QStringLiteral(".png");
    if (QFileInfo::exists(outPath))
        return outPath;

    QFileIconProvider provider;
    const QIcon icon = provider.icon(QFileInfo(executablePath));
    const QPixmap pixmap = icon.pixmap(64, 64);
    if (pixmap.isNull() || !pixmap.save(outPath, "png"))
        return {};
    return outPath;
}
