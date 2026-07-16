#include "capture/ScreenshotService.h"

#include "capture/CaptureUtil.h"
#include "config/ConfigKeys.h"
#include "config/ConfigManager.h"
#include "config/CaptureLocations.h"
#include "core/GameIdentity.h"
#include "games/GameDetector.h"

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QImage>
#include <QImageWriter>
#include <QRunnable>
#include <QThreadPool>

#include <windows.h>

ScreenshotService::ScreenshotService(ConfigManager* config, CaptureLocations* locations,
                                     QObject* parent)
    : QObject(parent)
    , m_config(config)
    , m_locations(locations)
{
}

void ScreenshotService::capture()
{
    const QString mode = m_config
        ? m_config->value(ConfigKeys::CaptureMode,
                          QStringLiteral("only_in_games")).toString()
        : QStringLiteral("only_in_games");

    const ForegroundGame g = GameDetector::current();
    if (!GameDetector::shouldCapture(g, mode)) {
        emit skipped(QStringLiteral("foreground window is not a game (mode=%1, process=%2)")
                         .arg(mode, g.processName.isEmpty() ? QStringLiteral("?") : g.processName));
        return;
    }

    QElapsedTimer t;
    t.start();
    const QImage img = grabRect(g.hwnd, g.x, g.y, g.w, g.h);
    const qint64 grabMs = t.elapsed();
    if (img.isNull()) {
        emit failed(QStringLiteral("GDI grab returned no pixels"));
        return;
    }

    // The pixels are captured. Fire instant feedback NOW (the shutter sound) and
    // hand the slow PNG encode + disk write to a worker thread, so pressing
    // Share never freezes the UI/game overlay (a 4K frame took ~2.7 s inline).
    emit grabbed();

    const QString gameName = g.gameName.isEmpty() ? QStringLiteral("Unknown Game")
                                                  : g.gameName;
    const QString executablePath = g.executablePath;
    const QString dir = m_locations->screenshotDir(gameName);
    // Read format/quality on the calling thread (ConfigManager is not meant for
    // concurrent access) and hand plain values to the worker.
    const bool jpeg = m_config
        && m_config->value(ConfigKeys::CaptureScreenshotFormat, QStringLiteral("png"))
               .toString().compare(QStringLiteral("jpg"), Qt::CaseInsensitive) == 0;
    const QString ext = jpeg ? QStringLiteral(".jpg") : QStringLiteral(".png");
    const int jpegQuality = m_config
        ? qBound(1, m_config->value(ConfigKeys::CaptureJpegQuality, 90).toInt(), 100)
        : 90;
    qInfo() << "Screenshot: grabbed" << img.width() << "x" << img.height()
            << "in" << grabMs << "ms — encoding in background";

    QThreadPool::globalInstance()->start(QRunnable::create(
        [this, img, gameName, executablePath, dir, jpeg, ext, jpegQuality]() {
        QElapsedTimer et;
        et.start();
        if (!QDir().mkpath(dir)) {
            emit failed(QStringLiteral("could not create ") + dir);
            return;
        }

        // Timestamped name with same-second clobber protection.
        const QString path = CaptureUtil::uniqueTimestampedPath(
            dir, QStringLiteral("yyyy-MM-dd_HH-mm-ss"), ext);

        QImageWriter writer(path, jpeg ? "JPG" : "PNG");
        if (jpeg)
            writer.setQuality(jpegQuality);
        else
            writer.setCompression(1);   // fast zlib level — encode speed over file size
        if (!writer.write(img)) {
            emit failed(QStringLiteral("could not write ") + path
                        + QStringLiteral(": ") + writer.errorString());
            return;
        }

        qInfo() << "Screenshot: saved" << path << "(" << img.width() << "x" << img.height()
                << ") encode+write" << et.elapsed() << "ms";
        emit captured(path, gameName, executablePath);
    }));
}

// Screen-DC BitBlt of the window's screen rectangle. CAPTUREBLT pulls in
// layered windows; whatever is composited on that region of the monitor is
// grabbed (correct for a foreground fullscreen/borderless game).
QImage ScreenshotService::grabRect(void* hwndPtr, int x, int y, int w, int h) const
{
    Q_UNUSED(hwndPtr);
    if (w <= 0 || h <= 0)
        return {};

    HDC screen = GetDC(nullptr);
    if (!screen)
        return {};

    HDC mem = CreateCompatibleDC(screen);
    HBITMAP bmp = CreateCompatibleBitmap(screen, w, h);
    QImage img;

    if (mem && bmp) {
        HGDIOBJ old = SelectObject(mem, bmp);
        if (BitBlt(mem, 0, 0, w, h, screen, x, y, SRCCOPY | CAPTUREBLT)) {
            BITMAPINFO bi = {};
            bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bi.bmiHeader.biWidth = w;
            bi.bmiHeader.biHeight = -h;   // top-down
            bi.bmiHeader.biPlanes = 1;
            bi.bmiHeader.biBitCount = 32;
            bi.bmiHeader.biCompression = BI_RGB;

            QImage buffer(w, h, QImage::Format_RGB32);   // 0xffRRGGBB == BGRA in memory
            if (GetDIBits(mem, bmp, 0, h, buffer.bits(), &bi, DIB_RGB_COLORS))
                img = buffer.copy();
        }
        SelectObject(mem, old);
    }

    if (bmp)
        DeleteObject(bmp);
    if (mem)
        DeleteDC(mem);
    ReleaseDC(nullptr, screen);
    return img;
}
