#include "config/Paths.h"

#include "config/LegacyMigration.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QStringList>

namespace
{
QString exeDir()
{
    return QCoreApplication::applicationDirPath();
}

// Portable mode: "portable.flag" next to the exe (dev builds) or one level up
// (dist layout, where the real exe lives in app/ and user data belongs at the
// package root — see docs/packaging.md). Empty string = not portable.
QString portableRoot()
{
    static const QString root = [] {
        const QString exe = exeDir();
        if (QFileInfo::exists(exe + QStringLiteral("/portable.flag")))
            return exe;
        const QString parent = QFileInfo(exe).absolutePath();
        if (QFileInfo::exists(parent + QStringLiteral("/portable.flag")))
            return parent;
        return QString();
    }();
    return root;
}
} // namespace

namespace Paths
{

bool isPortable()
{
    return !portableRoot().isEmpty();
}

QString dataDir()
{
    if (isPortable()) {
        return LegacyMigration::adoptDirectory(
            portableRoot() + QStringLiteral("/gamehq-data"),
            { portableRoot() + QStringLiteral("/saveplay-data"),
              portableRoot() + QStringLiteral("/playhq-data") });
    }
    const QString appData = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    return LegacyMigration::adoptDirectory(
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation),
        { appData + QStringLiteral("/SavePlay/SavePlay"),
          appData + QStringLiteral("/PlayHQ/PlayHQ") });
}

QString databasePath()
{
    const QString current = dataDir() + QStringLiteral("/gamehq.db");
    LegacyMigration::adoptFile(current, { dataDir() + QStringLiteral("/saveplay.db"),
                                          dataDir() + QStringLiteral("/playhq.db") });
    return current;
}

QString logsDir()        { return dataDir() + QStringLiteral("/logs"); }
QString thumbnailsDir()  { return dataDir() + QStringLiteral("/thumbnails"); }
QString gameIconsDir()   { return dataDir() + QStringLiteral("/game-icons"); }
QString replayCacheDir() { return dataDir() + QStringLiteral("/replay-cache"); }
QString soundPacksDir()  { return dataDir() + QStringLiteral("/sound-packs"); }

QString packageRoot()
{
    const QFileInfo appDir(exeDir());
    if (appDir.fileName().compare(QStringLiteral("app"), Qt::CaseInsensitive) == 0)
        return appDir.absolutePath();
    return exeDir();
}

QString capturesRoot()
{
    if (isPortable())
        return portableRoot() + QStringLiteral("/Captures");
    const QString movies = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
    // Existing capture roots remain where they are: capture paths are persisted
    // absolutely and user media is never moved automatically. New installs use
    // the GameHQ root; CaptureLocations keeps historical roots scanned.
    return LegacyMigration::preferExisting(movies + QStringLiteral("/GameHQ"),
                                           { movies + QStringLiteral("/SavePlay"),
                                             movies + QStringLiteral("/PlayHQ") });
}

QString toStoredPath(const QString& path)
{
    if (path.trimmed().isEmpty())
        return {};
    const QString absolute = QDir::cleanPath(QFileInfo(fromStoredPath(path)).absoluteFilePath());
    const QString root = portableRoot();
    if (root.isEmpty())
        return absolute;
    const QString relative = QDir(root).relativeFilePath(absolute);
    if (relative != QStringLiteral("..") && !relative.startsWith(QStringLiteral("../")))
        return QStringLiteral("portable:/") + relative;
    return absolute;
}

QString fromStoredPath(const QString& path)
{
    if (path.trimmed().isEmpty())
        return {};
    const QString clean = QDir::fromNativeSeparators(path.trimmed());
    if (clean.startsWith(QStringLiteral("portable:/"))) {
        const QString root = portableRoot();
        return root.isEmpty() ? clean : QDir::cleanPath(root + QLatin1Char('/') + clean.mid(10));
    }
    if (QDir::isRelativePath(clean) && !portableRoot().isEmpty())
        return QDir::cleanPath(portableRoot() + QLatin1Char('/') + clean);
    return QDir::cleanPath(clean);
}

QString repairMovedPath(const QString& path)
{
    if (path.trimmed().isEmpty())
        return {};
    const QString resolved = fromStoredPath(path);
    if (QFileInfo::exists(resolved))
        return resolved;

    const QString normalized = QDir::fromNativeSeparators(resolved);
    struct Rebase { const char* marker; QString root; };
    const Rebase roots[] = {
        { "/Captures/", capturesRoot() },
        { "/gamehq-data/thumbnails/", thumbnailsDir() },
        { "/gamehq-data/game-icons/", gameIconsDir() },
        { "/gamehq-data/replay-cache/", replayCacheDir() }
    };
    for (const Rebase& candidate : roots) {
        const QString marker = QString::fromLatin1(candidate.marker);
        const qsizetype at = normalized.lastIndexOf(marker, -1, Qt::CaseInsensitive);
        if (at < 0)
            continue;
        const QString rebased = QDir::cleanPath(candidate.root + QLatin1Char('/')
                                                + normalized.mid(at + marker.size()));
        if (QFileInfo::exists(rebased))
            return rebased;
    }
    return resolved;
}

void ensureDirectories()
{
    for (const QString& dir : { dataDir(), logsDir(), thumbnailsDir(), gameIconsDir(),
                                replayCacheDir(), soundPacksDir(), capturesRoot() })
        QDir().mkpath(dir);
}

} // namespace Paths
