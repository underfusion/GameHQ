#pragma once
#include <QString>

// Thumbnail generation + cache (gamehq-data/thumbnails/<md5>.jpg).
// Images are decoded with QImageReader; video thumbnails use the existing
// Media Foundation first-frame decoder on a short-lived MTA worker thread.
namespace ThumbnailService
{
    // Returns cached/created thumbnail path, or "" if not producible.
    QString ensureThumbnail(const QString& filePath, const QString& type,
                            const QString& thumbnailsDir);
}
