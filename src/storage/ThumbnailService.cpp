#include "storage/ThumbnailService.h"
#include "capture/ReplayExporter.h"

#include <QCryptographicHash>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QDir>
#include <QThread>

#ifdef Q_OS_WIN
#include <objbase.h>
#endif

namespace ThumbnailService
{

QString ensureThumbnail(const QString& filePath, const QString& type,
                        const QString& thumbnailsDir)
{
    const QString hash = QString::fromLatin1(
        QCryptographicHash::hash(filePath.toUtf8(), QCryptographicHash::Md5).toHex());
    const QString thumbPath = thumbnailsDir + QLatin1Char('/') + hash + QLatin1String(".jpg");

    if (QFileInfo::exists(thumbPath))
        return thumbPath;

    QDir().mkpath(thumbnailsDir);
    if (type == QLatin1String("video")) {
        // Early replay builds named previews after the clip instead of the
        // path hash. Reattach those orphaned previews before decoding again.
        const QString legacyThumb = thumbnailsDir + QLatin1Char('/')
                                    + QFileInfo(filePath).completeBaseName()
                                    + QStringLiteral("_clip.png");
        if (QFileInfo::exists(legacyThumb))
            return legacyThumb;

        bool saved = false;
        QThread* worker = QThread::create([&] {
#ifdef Q_OS_WIN
            const HRESULT initialized = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
#endif
            QImage frame;
            saved = ReplayExporter::grabThumbnail(filePath, frame)
                    && !frame.isNull() && frame.save(thumbPath, "JPG", 85);
#ifdef Q_OS_WIN
            if (SUCCEEDED(initialized))
                CoUninitialize();
#endif
        });
        worker->start();
        worker->wait();
        delete worker;
        return saved ? thumbPath : QString();
    }
    if (type != QLatin1String("screenshot"))
        return {};

    QImageReader reader(filePath);
    reader.setAutoTransform(true);
    // Decode downscaled to keep the scan pass fast on 4K screenshots.
    const QSize original = reader.size();
    if (original.isValid() && original.width() > 512)
        reader.setScaledSize(QSize(512, qMax(1, original.height() * 512 / original.width())));

    const QImage image = reader.read();
    if (image.isNull())
        return {};
    if (!image.save(thumbPath, "JPG", 85))
        return {};
    return thumbPath;
}

} // namespace ThumbnailService
