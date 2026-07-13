#include "storage/CaptureScanner.h"
#include "storage/CaptureDatabase.h"
#include "storage/ThumbnailService.h"
#include "config/CaptureLocations.h"

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
    int added = 0;
    for (const QString& root : m_locations->managedRoots())
        added += scanFolder(root, QStringLiteral("GameHQ"));
    const QStringList watched = m_db->watchedFolders();
    for (const QString& folder : watched)
        added += scanFolder(folder, QStringLiteral("Imported"));
    qInfo() << "Scan: finished," << added << "new capture(s)";
    emit scanFinished(added);
    return added;
}

int CaptureScanner::scanFolder(const QString& root, const QString& source)
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

        if (m_db->hasCapture(path)) {
            const QString existingThumb = m_db->thumbnailForCapture(path);
            if (!QFileInfo::exists(existingThumb)) {
                const QString thumb = ThumbnailService::ensureThumbnail(path, type, m_thumbnailsDir);
                if (!thumb.isEmpty())
                    m_db->setThumbnailForCapture(path, thumb);
            }
            continue;
        }

        const QString game = inferGameName(root, path);
        const QString createdAt =
            info.birthTime().isValid() ? info.birthTime().toUTC().toString(Qt::ISODate)
                                       : info.lastModified().toUTC().toString(Qt::ISODate);
        const int id = m_db->insertCapture(path, type, game, createdAt, source);
        if (id < 0)
            continue;

        const QString thumb = ThumbnailService::ensureThumbnail(path, type, m_thumbnailsDir);
        if (!thumb.isEmpty())
            m_db->setThumbnail(id, thumb);
        ++added;
    }
    return added;
}

QString CaptureScanner::inferGameName(const QString& root, const QString& filePath) const
{
    const QDir rootDir(root);
    const QString relative = rootDir.relativeFilePath(filePath);
    const QStringList parts = relative.split(QLatin1Char('/'), Qt::SkipEmptyParts);

    // "<Game>/Screenshots/file.png" or "<Game>/Clips/file.mp4"
    if (parts.size() >= 3)
        return parts.first();
    // "<Game>/file.png"
    if (parts.size() == 2)
        return parts.first();
    return QStringLiteral("Unknown Game");
}
