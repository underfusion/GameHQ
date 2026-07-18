#include "config/LegacyMigration.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

namespace LegacyMigration
{

QString adoptDirectory(const QString& current, const QStringList& legacyPaths)
{
    for (const QString& legacy : legacyPaths) {
        if (!QFileInfo::exists(current) && QFileInfo::exists(legacy))
            QDir().rename(legacy, current);
        // Re-checked: a failed rename must not strand the old data.
        if (!QFileInfo::exists(current) && QFileInfo::exists(legacy))
            return legacy;
    }
    return current;
}

void adoptFile(const QString& current, const QStringList& legacyPaths)
{
    for (const QString& legacy : legacyPaths) {
        if (!QFile::exists(current) && QFile::exists(legacy))
            QFile::rename(legacy, current);
    }
}

QString preferExisting(const QString& current, const QStringList& legacyPaths)
{
    if (QFileInfo::exists(current))
        return current;
    for (const QString& legacy : legacyPaths) {
        if (QFileInfo::exists(legacy))
            return legacy;
    }
    return current;
}

} // namespace LegacyMigration
