#pragma once
#include <QByteArray>
#include <QString>

class QImage;

// Thumbnail generation + cache (gamehq-data/thumbnails/<md5>.jpg).
// Images are decoded with QImageReader; video thumbnails use the existing
// Media Foundation first-frame decoder on a short-lived MTA worker thread.
namespace ThumbnailService
{
    // Rejects missing, empty and undecodable cached thumbnails.
    bool isUsableThumbnail(const QString& thumbnailPath);

    // Writes through QSaveFile so interruption cannot replace a valid cache
    // entry with a truncated or zero-filled image.
    bool saveThumbnail(const QImage& image, const QString& thumbnailPath,
                       const QByteArray& format, int quality = -1);

    // Returns cached/created thumbnail path, or "" if not producible.
    QString ensureThumbnail(const QString& filePath, const QString& type,
                            const QString& thumbnailsDir);
}
