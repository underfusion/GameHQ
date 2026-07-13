#pragma once

#include <QString>
#include <QVariantList>

class CaptureDatabase;
class GalleryModel;

class CaptureLibraryService
{
public:
    CaptureLibraryService(CaptureDatabase* db, GalleryModel* gallery, GalleryModel* overlayGallery);

    bool deleteCapture(GalleryModel* model, int row);
    bool deleteCaptures(GalleryModel* model, const QVariantList& rows);
    void openCapture(GalleryModel* model, int row) const;
    void showInFolder(GalleryModel* model, int row) const;

    void commitCapture(const QString& filePath, const QString& type,
                       const QString& gameName, const QString& executablePath = QString());
    void commitClip(const QString& filePath, const QString& gameName,
                    const QString& thumbnailPath, const QString& executablePath = QString());

private:
    void refreshGalleries();

    CaptureDatabase* m_db;
    GalleryModel* m_gallery;
    GalleryModel* m_overlayGallery;
};
