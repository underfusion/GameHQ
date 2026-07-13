#include "capture/ReplayExporter.h"

#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <mfobjects.h>   // IMF2DBuffer

#include <QImage>
#include <QDebug>
#include <QElapsedTimer>
#include <QFileInfo>

#include <cstring>
#include <string>
#include <vector>

namespace {

// Copy the compressed H.264 samples of N MP4 segments into one MP4, stitching
// timestamps so playback is continuous. No decode, no encode. Every input must
// share identical encode params and start on an IDR keyframe.
QString fromWide(const std::wstring& value)
{
    return QString::fromStdWString(value);
}

bool remuxConcatImpl(const std::vector<std::wstring>& inFiles, const std::wstring& outFile,
                     const QString& traceId)
{
    QElapsedTimer totalTimer;
    totalTimer.start();
    const QString tag = traceId.isEmpty() ? QStringLiteral("no-id") : traceId;

    if (inFiles.empty()) {
        qWarning().noquote() << QStringLiteral("ReplaySave[%1]: concat refused - no input segments")
                                    .arg(tag);
        return false;
    }

    const bool startedMf = SUCCEEDED(MFStartup(MF_VERSION, MFSTARTUP_LITE));
    qInfo().noquote() << QStringLiteral("ReplaySave[%1]: concat begin inputs=%2 output=%3 mfStarted=%4")
                             .arg(tag).arg(inFiles.size()).arg(fromWide(outFile))
                             .arg(startedMf ? QStringLiteral("yes") : QStringLiteral("no"));

    IMFSinkWriter* writer     = nullptr;
    DWORD          outVideoStream = 0;
    DWORD          outAudioStream = 0;
    bool           haveWriter = false;
    bool           haveAudio = false;
    LONGLONG       offset        = 0;       // cumulative 100-ns offset across segments
    LONGLONG       frameDurGuess = 333667;  // ~30fps fallback if a sample lacks duration
    bool           wroteAnything = false;
    bool           skippedSegment = false;
    bool           writeFailed = false;
    HRESULT        hr = S_OK;

    int segmentIndex = 0;
    for (const std::wstring& path : inFiles) {
        QElapsedTimer segTimer;
        segTimer.start();
        const QString qPath = fromWide(path);
        const QFileInfo fi(qPath);
        qInfo().noquote() << QStringLiteral("ReplaySave[%1]: segment %2 open path=%3 exists=%4 bytes=%5")
                                 .arg(tag).arg(segmentIndex).arg(qPath)
                                 .arg(fi.exists() ? QStringLiteral("yes") : QStringLiteral("no"))
                                 .arg(fi.exists() ? fi.size() : -1);

        // Do NOT enable advanced video processing => reader keeps the native
        // (compressed) type => hands back undecoded H.264 samples.
        IMFAttributes* rattr = nullptr;
        MFCreateAttributes(&rattr, 1);
        rattr->SetUINT32(MF_SOURCE_READER_ENABLE_ADVANCED_VIDEO_PROCESSING, FALSE);

        IMFSourceReader* reader = nullptr;
        hr = MFCreateSourceReaderFromURL(path.c_str(), rattr, &reader);
        if (rattr) rattr->Release();
        // Keep scanning so all bad inputs are logged, but report the export as
        // failed at the end. A partial 10-second clip is worse than a clear
        // failure when the configured ring should contain 30+ seconds.
        if (FAILED(hr) || !reader) {
            qWarning().nospace() << "ReplaySave[" << tag
                                 << "]: segment " << segmentIndex
                                 << " skipped - open failed hr=0x"
                                 << Qt::hex << quint32(hr);
            skippedSegment = true;
            ++segmentIndex;
            continue;
        }

        reader->SetStreamSelection((DWORD)MF_SOURCE_READER_ALL_STREAMS,        FALSE);
        reader->SetStreamSelection((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, TRUE);
        reader->SetStreamSelection((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, TRUE);

        // Pin the native compressed type => no decoder inserted (passthrough).
        IMFMediaType* nativeVideoType = nullptr;
        hr = reader->GetNativeMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, &nativeVideoType);
        if (FAILED(hr) || !nativeVideoType) {
            qWarning() << "ReplayExporter: skipping segment — no native video type";
            skippedSegment = true;
            reader->Release();
            continue;
        }
        reader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr, nativeVideoType);

        IMFMediaType* nativeAudioType = nullptr;
        const bool segHasAudio =
            SUCCEEDED(reader->GetNativeMediaType((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, &nativeAudioType))
            && nativeAudioType;
        if (segHasAudio)
            reader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, nativeAudioType);

        UINT32 segW = 0, segH = 0, fpsNum = 0, fpsDen = 0;
        MFGetAttributeSize(nativeVideoType, MF_MT_FRAME_SIZE, &segW, &segH);
        MFGetAttributeRatio(nativeVideoType, MF_MT_FRAME_RATE, &fpsNum, &fpsDen);
        qInfo().noquote() << QStringLiteral("ReplaySave[%1]: segment %2 media video=%3x%4 fps=%5/%6 audio=%7")
                                 .arg(tag).arg(segmentIndex)
                                 .arg(segW).arg(segH).arg(fpsNum).arg(fpsDen)
                                 .arg(segHasAudio ? QStringLiteral("yes") : QStringLiteral("no"));

        if (!haveWriter) {
            IMFAttributes* wattr = nullptr;
            MFCreateAttributes(&wattr, 2);
            wattr->SetGUID  (MF_TRANSCODE_CONTAINERTYPE,        MFTranscodeContainerType_MPEG4);
            wattr->SetUINT32(MF_SINK_WRITER_DISABLE_THROTTLING, TRUE);

            hr = MFCreateSinkWriterFromURL(outFile.c_str(), nullptr, wattr, &writer);
            if (wattr) wattr->Release();
            if (FAILED(hr) || !writer) { nativeVideoType->Release(); if (nativeAudioType) nativeAudioType->Release(); reader->Release(); break; }

            // Clone the FULL native type so MF_MT_MPEG_SEQUENCE_HEADER (SPS/PPS),
            // frame size, PAR, etc. reach the output avcC box. A hand-built minimal
            // type would yield an unplayable file.
            IMFMediaType* outType = nullptr;
            MFCreateMediaType(&outType);
            nativeVideoType->CopyAllItems(outType);

            hr = writer->AddStream(outType, &outVideoStream);
            if (SUCCEEDED(hr))
                hr = writer->SetInputMediaType(outVideoStream, outType, nullptr); // same type => no encoder
            outType->Release();
            if (FAILED(hr)) { nativeVideoType->Release(); if (nativeAudioType) nativeAudioType->Release(); reader->Release(); break; }

            if (segHasAudio) {
                IMFMediaType* outAudioType = nullptr;
                MFCreateMediaType(&outAudioType);
                nativeAudioType->CopyAllItems(outAudioType);
                hr = writer->AddStream(outAudioType, &outAudioStream);
                if (SUCCEEDED(hr))
                    hr = writer->SetInputMediaType(outAudioStream, outAudioType, nullptr);
                outAudioType->Release();
                if (SUCCEEDED(hr))
                    haveAudio = true;
                else
                    qWarning().nospace() << "ReplayExporter: audio stream copy disabled hr=0x"
                                         << Qt::hex << quint32(hr);
            }

            UINT32 num = 0, den = 0;
            if (SUCCEEDED(MFGetAttributeRatio(nativeVideoType, MF_MT_FRAME_RATE, &num, &den)) && num)
                frameDurGuess = (LONGLONG)(10000000.0 * den / num);

            hr = writer->BeginWriting();
            if (FAILED(hr)) { nativeVideoType->Release(); if (nativeAudioType) nativeAudioType->Release(); reader->Release(); break; }
            haveWriter = true;
        }
        nativeVideoType->Release();
        if (nativeAudioType)
            nativeAudioType->Release();

        LONGLONG segMaxEnd  = 0;
        bool     firstOfSeg = true;
        bool     wroteThisSegment = false;
        int      videoSamples = 0;
        int      audioSamples = 0;
        for (;;) {
            DWORD      streamFlags = 0;
            LONGLONG   ts          = 0;
            IMFSample* sample      = nullptr;
            hr = reader->ReadSample((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM,
                                    0, nullptr, &streamFlags, &ts, &sample);
            if (FAILED(hr)) {
                qWarning().nospace() << "ReplayExporter: video read failed hr=0x"
                                     << Qt::hex << quint32(hr);
                skippedSegment = true;
                break;
            }
            if (streamFlags & MF_SOURCE_READERF_ENDOFSTREAM) { if (sample) sample->Release(); break; }
            if (!sample) continue;   // stream tick / gap, no payload

            LONGLONG dur = 0;
            if (FAILED(sample->GetSampleDuration(&dur)) || dur <= 0)
                dur = frameDurGuess;

            sample->SetSampleTime(ts + offset);        // make timeline continuous
            if (firstOfSeg) {                          // each seg starts with IDR
                sample->SetUINT32(MFSampleExtension_Discontinuity, TRUE);
                firstOfSeg = false;
            }
            hr = writer->WriteSample(outVideoStream, sample);
            if (FAILED(hr)) {
                qWarning().nospace() << "ReplayExporter: video write failed hr=0x"
                                     << Qt::hex << quint32(hr);
                writeFailed = true;
                sample->Release();
                break;
            }

            const LONGLONG end = ts + dur;
            if (end > segMaxEnd) segMaxEnd = end;
            sample->Release();
            wroteAnything = true;
            wroteThisSegment = true;
            ++videoSamples;
        }

        if (!writeFailed && haveAudio && segHasAudio) {
            bool firstAudioOfSeg = true;
            for (;;) {
                DWORD      streamFlags = 0;
                LONGLONG   ts          = 0;
                IMFSample* sample      = nullptr;
                hr = reader->ReadSample((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM,
                                        0, nullptr, &streamFlags, &ts, &sample);
                if (FAILED(hr)) {
                    qWarning().nospace() << "ReplayExporter: audio read failed hr=0x"
                                         << Qt::hex << quint32(hr);
                    writeFailed = true;
                    break;
                }
                if (streamFlags & MF_SOURCE_READERF_ENDOFSTREAM) { if (sample) sample->Release(); break; }
                if (!sample) continue;

                LONGLONG dur = 0;
                if (FAILED(sample->GetSampleDuration(&dur)) || dur <= 0)
                    dur = 213333; // ~1024 samples @ 48kHz fallback

                sample->SetSampleTime(ts + offset);
                if (firstAudioOfSeg) {
                    sample->SetUINT32(MFSampleExtension_Discontinuity, TRUE);
                    firstAudioOfSeg = false;
                }
                hr = writer->WriteSample(outAudioStream, sample);
                if (FAILED(hr)) {
                    qWarning().nospace() << "ReplayExporter: audio write failed hr=0x"
                                         << Qt::hex << quint32(hr);
                    writeFailed = true;
                    sample->Release();
                    break;
                }

                const LONGLONG end = ts + dur;
                if (end > segMaxEnd) segMaxEnd = end;
                sample->Release();
                ++audioSamples;
            }
        }

        if (!wroteThisSegment) {
            skippedSegment = true;
            qWarning().noquote() << QStringLiteral("ReplaySave[%1]: segment %2 wrote no video samples")
                                        .arg(tag).arg(segmentIndex);
        }
        qInfo().noquote() << QStringLiteral("ReplaySave[%1]: segment %2 done videoSamples=%3 audioSamples=%4 durationMs=%5 elapsedMs=%6")
                                 .arg(tag).arg(segmentIndex)
                                 .arg(videoSamples).arg(audioSamples)
                                 .arg(segMaxEnd / 10000).arg(segTimer.elapsed());
        offset += segMaxEnd;   // next segment picks up where this one ended
        reader->Release();
        if (writeFailed)
            break;
        ++segmentIndex;
    }

    if (writer && haveWriter && wroteAnything)
        hr = writer->Finalize();               // writes moov -> playable file
    else
        hr = E_FAIL;

    if (writer) writer->Release();
    if (startedMf) MFShutdown();
    const bool ok = SUCCEEDED(hr) && wroteAnything && !skippedSegment && !writeFailed;
    qInfo().noquote() << QStringLiteral("ReplaySave[%1]: concat end ok=%2 wroteAnything=%3 skipped=%4 writeFailed=%5 finalHr=0x%6 durationMs=%7 totalMs=%8")
                             .arg(tag)
                             .arg(ok ? QStringLiteral("yes") : QStringLiteral("no"))
                             .arg(wroteAnything ? QStringLiteral("yes") : QStringLiteral("no"))
                             .arg(skippedSegment ? QStringLiteral("yes") : QStringLiteral("no"))
                             .arg(writeFailed ? QStringLiteral("yes") : QStringLiteral("no"))
                             .arg(QString::number(quint32(hr), 16))
                             .arg(offset / 10000)
                             .arg(totalTimer.elapsed());
    return ok;
}

// Decode the first frame of an MP4 to RGB32 (== BGRA == QImage::Format_RGB32).
bool grabThumbnailImpl(const std::wstring& file, QImage& out)
{
    const bool startedMf = SUCCEEDED(MFStartup(MF_VERSION, MFSTARTUP_LITE));

    IMFAttributes* rattr = nullptr;
    MFCreateAttributes(&rattr, 1);
    rattr->SetUINT32(MF_SOURCE_READER_ENABLE_ADVANCED_VIDEO_PROCESSING, TRUE);

    IMFSourceReader* reader = nullptr;
    HRESULT hr = MFCreateSourceReaderFromURL(file.c_str(), rattr, &reader);
    if (rattr) rattr->Release();
    if (FAILED(hr) || !reader) { if (startedMf) MFShutdown(); return false; }

    reader->SetStreamSelection((DWORD)MF_SOURCE_READER_ALL_STREAMS,        FALSE);
    reader->SetStreamSelection((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, TRUE);

    IMFMediaType* want = nullptr;
    MFCreateMediaType(&want);
    want->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    want->SetGUID(MF_MT_SUBTYPE,    MFVideoFormat_RGB32);
    hr = reader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr, want);
    want->Release();
    if (FAILED(hr)) { reader->Release(); if (startedMf) MFShutdown(); return false; }

    UINT32 width = 0, height = 0;
    LONG   defStride = 0;
    IMFMediaType* cur = nullptr;
    if (SUCCEEDED(reader->GetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, &cur))) {
        MFGetAttributeSize(cur, MF_MT_FRAME_SIZE, &width, &height);
        UINT32 s = 0;
        if (SUCCEEDED(cur->GetUINT32(MF_MT_DEFAULT_STRIDE, &s)))
            defStride = (LONG)s;
        else
            MFGetStrideForBitmapInfoHeader(MFVideoFormat_RGB32.Data1, width, &defStride);
        cur->Release();
    }
    if (width == 0 || height == 0) { reader->Release(); if (startedMf) MFShutdown(); return false; }

    IMFSample* sample = nullptr;
    for (;;) {
        DWORD    flags = 0;
        LONGLONG ts    = 0;
        hr = reader->ReadSample((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM,
                                0, nullptr, &flags, &ts, &sample);
        if (FAILED(hr) || (flags & MF_SOURCE_READERF_ENDOFSTREAM)) break;
        if (sample) break;                 // got a decoded frame
    }
    if (!sample) { reader->Release(); if (startedMf) MFShutdown(); return false; }

    IMFMediaBuffer* buf = nullptr;
    hr = sample->ConvertToContiguousBuffer(&buf);   // flattens multi-buffer samples
    if (FAILED(hr) || !buf) { sample->Release(); reader->Release(); if (startedMf) MFShutdown(); return false; }

    bool okResult = false;

    IMF2DBuffer* buf2d = nullptr;
    if (SUCCEEDED(buf->QueryInterface(IID_IMF2DBuffer, (void**)&buf2d)) && buf2d) {
        BYTE* scan0 = nullptr;
        LONG  pitch = 0;
        if (SUCCEEDED(buf2d->Lock2D(&scan0, &pitch))) {
            QImage img((int)width, (int)height, QImage::Format_RGB32);
            const bool bottomUp = pitch < 0;
            const LONG absPitch = pitch < 0 ? -pitch : pitch;
            for (UINT y = 0; y < height; ++y) {
                const BYTE* srcRow = bottomUp
                    ? scan0 - (LONG)y * absPitch
                    : scan0 + (LONG)y * absPitch;
                memcpy(img.scanLine((int)y), srcRow, (size_t)width * 4);
            }
            out = img.copy();                                // detach before Unlock
            okResult = true;
            buf2d->Unlock2D();
        }
        buf2d->Release();
    } else {
        BYTE* data = nullptr; DWORD len = 0;
        if (SUCCEEDED(buf->Lock(&data, nullptr, &len))) {
            const bool bottomUp = defStride < 0;
            const LONG absStride = defStride < 0 ? -defStride : (defStride ? defStride : (LONG)width * 4);
            QImage img((int)width, (int)height, QImage::Format_RGB32);
            for (UINT y = 0; y < height; ++y) {
                const UINT srcY = bottomUp ? (height - 1 - y) : y;
                memcpy(img.scanLine((int)y), data + (size_t)srcY * absStride, (size_t)width * 4);
            }
            out = img.copy();
            okResult = true;
            buf->Unlock();
        }
    }

    buf->Release();
    sample->Release();
    reader->Release();
    if (startedMf) MFShutdown();
    return okResult;
}

} // namespace

namespace ReplayExporter {

bool concat(const QStringList& inputFiles, const QString& outputFile, const QString& traceId)
{
    std::vector<std::wstring> in;
    in.reserve(inputFiles.size());
    for (const QString& f : inputFiles)
        in.push_back(f.toStdWString());
    return remuxConcatImpl(in, outputFile.toStdWString(), traceId);
}

bool grabThumbnail(const QString& file, QImage& out)
{
    return grabThumbnailImpl(file.toStdWString(), out);
}

} // namespace ReplayExporter
