#pragma once

#include <QString>
#include <QStringList>

class QImage;

// 0.5 Step 8 — turn buffered replay segments into a final saved clip.
//
// concat(): remux-concat H.264 fragmented-MP4 segments (in chronological order)
// into ONE standard MP4, copying the compressed samples with stitched timestamps
// — NO re-encode. All inputs must share identical encode params (our SegmentRecorder
// guarantees this) and start on an IDR keyframe.
//
// grabThumbnail(): decode the first frame of an MP4 to a BGRA QImage for a gallery
// video thumbnail.
//
// Both use Media Foundation and must run on an MTA thread (the frame-pump worker).
namespace ReplayExporter
{
    bool concat(const QStringList& inputFiles, const QString& outputFile,
                const QString& traceId = QString());
    bool grabThumbnail(const QString& file, QImage& out);
}
