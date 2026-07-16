#include "storage/CaptureScanner.h"
#include "storage/CaptureDatabase.h"
#include "storage/ThumbnailService.h"
#include "config/CaptureLocations.h"
#include "core/GameIdentity.h"

#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QDebug>

namespace
{
const QStringList kImageSuffixes = { "png", "jpg", "jpeg", "bmp", "webp" };
const QStringList kVideoSuffixes = { "mp4", "mkv", "mov", "avi", "webm" };

QString typeForSuffix(const QString& suffix)
{
    if (kImageSuffixes.contains(suffix))
        return QStringLiteral("screenshot");
    if (kVideoSuffixes.contains(suffix))
        return QStringLiteral("video");
    return {};
}
} // namespace

CaptureScanner::CaptureScanner(CaptureDatabase* db, CaptureLocations* locations,
                               QString thumbnailsDir, QObject* parent)
    : QObject(parent)
    , m_db(db)
    , m_locations(locations)
    , m_thumbnailsDir(std::move(thumbnailsDir))
{
}

int CaptureScanner::scanAll()
{
    // One snapshot of the captures table up front; the walk below then diffs
    // against memory instead of issuing two queries per file on disk.
    QHash<QString, CaptureIndexEntry> index = m_db->captureIndex();

    int added = 0;
    for (const QString& root : m_locations->managedRoots())
        added += scanFolder(root, QStringLiteral("GameHQ"), index);
    const QStringList watched = m_db->watchedFolders();
    for (const QString& folder : watched)
        added += scanFolder(folder, QStringLiteral("Imported"), index);
    qInfo() << "Scan: finished," << added << "new capture(s), indexed" << index.size()
            << "known path(s)";
    emit scanFinished(added);
    return added;
}

int CaptureScanner::scanFolder(const QString& root, const QString& source,
                               QHash<QString, CaptureIndexEntry>& index)
{
    if (root.isEmpty() || !QDir(root).exists())
        return 0;

    int added = 0;
    QDirIterator it(root, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString path = QDir::cleanPath(it.next());
        const QFileInfo info(path);
        const QString type = typeForSuffix(info.suffix().toLower());
        if (type.isEmpty())
            continue;

        const QString key = CaptureDatabase::storedPathKey(path);
        const auto known = index.constFind(key);
        if (known != index.constEnd()) {
            if (!QFileInfo::exists(known->thumbnailPath)) {
                const QString thumb = ThumbnailService::ensureThumbnail(path, type, m_thumbnailsDir);
                if (!thumb.isEmpty() && m_db->setThumbnailForCapture(path, thumb))
                    index[key].thumbnailPath = thumb;
            }
            continue;
        }

        const QString game = GameIdentity::inferFromPath(root, path);
        const QString createdAt =
            info.birthTime().isValid() ? info.birthTime().toUTC().toString(Qt::ISODate)
                                       : info.lastModified().toUTC().toString(Qt::ISODate);
        const int id = m_db->insertCapture(path, type, game, createdAt, source);
        if (id < 0)
            continue;

        const QString thumb = ThumbnailService::ensureThumbnail(path, type, m_thumbnailsDir);
        if (!thumb.isEmpty())
            m_db->setThumbnail(id, thumb);
        // Keep the snapshot authoritative for the rest of the walk, so a path
        // reachable from two roots is not inserted twice.
        index.insert(key, { thumb, false });
        ++added;
    }
    return added;
}

