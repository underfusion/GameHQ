#pragma once
#include <QString>
#include <QStringList>

// Compatibility shims for the app's two former names (SavePlay → PlayHQ →
// GameHQ). Old installs still have data under the legacy names; these helpers
// adopt it on first start so nobody's library is stranded (docs/storage.md).
//
// Legacy lists are always ordered newest-first (playhq before saveplay only
// where an install could plausibly have both) and every helper is a no-op once
// the current path exists, so they stay cheap on every later start.
namespace LegacyMigration
{
    // Renames the first existing legacy directory onto `current` when `current`
    // is missing. Returns the directory to actually use: `current` normally, or
    // the legacy path when the rename could not be performed (e.g. locked by
    // another process) so the old data is still read in place.
    QString adoptDirectory(const QString& current, const QStringList& legacyPaths);

    // File counterpart of adoptDirectory, without the fallback: a legacy file is
    // renamed onto `current` when `current` does not exist yet.
    void adoptFile(const QString& current, const QStringList& legacyPaths);

    // Read-only variant: picks the first existing legacy path when `current`
    // does not exist, renaming nothing. Used for the capture root, where user
    // media is never moved automatically.
    QString preferExisting(const QString& current, const QStringList& legacyPaths);
}
