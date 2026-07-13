#pragma once
#include <QString>

// Resolves all data locations. Portable mode = "portable.flag" file next to the exe
// or in its parent directory (dist layout keeps the exe in app/ and the data at
// the package root — docs/storage.md, docs/packaging.md). All getters return
// absolute paths without trailing slash.
namespace Paths
{
    bool isPortable();

    QString dataDir();        // config.json, gamehq.db
    QString logsDir();
    QString thumbnailsDir();
    QString gameIconsDir();
    QString replayCacheDir();
    QString soundPacksDir();
    QString capturesRoot();   // per-game capture tree

    // Portable persistence. Values below the package root are stored as
    // "portable:/..." and resolved against the package's current location.
    QString toStoredPath(const QString& path);
    QString fromStoredPath(const QString& path);
    QString repairMovedPath(const QString& path);

    // Creates every directory above if missing. Call once at startup.
    void ensureDirectories();
}
