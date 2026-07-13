#include "ui/CaptureLibraryService.h"

#include "config/Paths.h"
#include "storage/CaptureDatabase.h"
#include "storage/ThumbnailService.h"
#include "ui/GalleryModel.h"
#include "ui/ShellActions.h"

#include <QDateTime>
#include <QFile>
#include <QList>
#include <QDebug>
#include <algorithm>

CaptureLibraryService::CaptureLibraryService(CaptureDatabase* db, GalleryModel* gallery,
                                             GalleryModel* overlayGallery)
    : m_db(db)
    , m_gallery(gallery)
    , m_overlayGallery(overlayGallery)
{
}

bool CaptureLibraryService::deleteCapture(GalleryModel* model, int row)
{
    if (!model)
        return false;
    const CaptureRecord* r = model->record(row);
    if (!r)
        return false;

    const int id = r->id;
    const QString file = r->filePath;
    const QString thumb = r->thumbnailPath;

    if (!file.isEmpty() && !QFile::remove(file))
        qWarning() << "Delete: could not remove file" << file;
    if (!thumb.isEmpty())
        QFile::remove(thumb);

    const bool deleted = m_db->deleteCapture(id);
    refreshGalleries();
    qInfo() << "Deleted capture" << id << file;
    return deleted;
}

bool CaptureLibraryService::deleteCaptures(GalleryModel* model, const QVariantList& rows)
{
    if (!model)
        return false;

    struct Item { int row; int id; QString file; QString thumb; };
    QList<Item> items;
    items.reserve(rows.size());
    for (const QVariant& v : rows) {
        bool ok = false;
        const int row = v.toInt(&ok);
        if (!ok)
            continue;
        const CaptureRecord* r = model->record(row);
        if (!r)
            continue;
        items.append({ row, r->id, r->filePath, r->thumbnailPath });
    }
    if (items.isEmpty())
        return false;

    std::sort(items.begin(), items.end(),
              [](const Item& a, const Item& b) { return a.row > b.row; });

    int removed = 0;
    for (const Item& it : items) {
        if (!it.file.isEmpty() && !QFile::remove(it.file))
            qWarning() << "Bulk delete: could not remove file" << it.file;
        if (!it.thumb.isEmpty())
            QFile::remove(it.thumb);
        if (m_db->deleteCapture(it.id))
            ++removed;
    }

    refreshGalleries();
    qInfo() << "Bulk-deleted" << removed << "captures";
    return true;
}

void CaptureLibraryService::openCapture(GalleryModel* model, int row) const
{
    if (!model)
        return;
    if (const CaptureRecord* r = model->record(row))
        ShellActions::openFile(r->filePath);
}

void CaptureLibraryService::showInFolder(GalleryModel* model, int row) const
{
    if (!model)
        return;
    if (const CaptureRecord* r = model->record(row))
        ShellActions::showInFolder(r->filePath);
}

void CaptureLibraryService::commitCapture(const QString& filePath, const QString& type,
                                          const QString& gameName, const QString& executablePath)
{
    const int id = m_db->insertCapture(filePath, type, gameName,
                                       QDateTime::currentDateTime().toString(Qt::ISODate),
                                       QStringLiteral("GameHQ"), executablePath);
    if (id > 0) {
        const QString thumb = ThumbnailService::ensureThumbnail(
            filePath, type, Paths::thumbnailsDir());
        if (!thumb.isEmpty())
            m_db->setThumbnail(id, thumb);
    }
    refreshGalleries();
}

void CaptureLibraryService::commitClip(const QString& filePath, const QString& gameName,
                                       const QString& thumbnailPath, const QString& executablePath)
{
    const int id = m_db->insertCapture(filePath, QStringLiteral("video"), gameName,
                                       QDateTime::currentDateTime().toString(Qt::ISODate),
                                       QStringLiteral("GameHQ"), executablePath);
    if (id > 0 && !thumbnailPath.isEmpty())
        m_db->setThumbnail(id, thumbnailPath);
    refreshGalleries();
}

void CaptureLibraryService::refreshGalleries()
{
    if (m_gallery)
        m_gallery->refresh();
    if (m_overlayGallery)
        m_overlayGallery->refresh();
}
