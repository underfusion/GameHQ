#pragma once
#include <QObject>
#include <QString>
#include <atomic>

class QImage;
class ConfigManager;
class CaptureLocations;

// Grabs a screenshot of the foreground game and saves it as PNG under the
// effective screenshot root: <root>/<Game>/Screenshots/<timestamp>.png.
//
// 0.4 path = GDI screen-DC BitBlt (works for borderless / windowed games and
// the desktop; exclusive-fullscreen DX games may come back black — that is the
// known GDI limitation the Windows Graphics Capture path replaces in 0.5,
// docs/capture-engine.md). DB insert / thumbnail / sound / notification are
// handled by the caller via the captured() signal.
class ScreenshotService : public QObject
{
    Q_OBJECT
public:
    ScreenshotService(ConfigManager* config, CaptureLocations* locations,
                      QObject* parent = nullptr);
    bool busy() const { return m_pendingWrites.load() > 0; }

public slots:
    void capture();   // grab per capture.mode; emits exactly one result signal
    // Save an already-grabbed image (e.g. a clip frame from the QML video
    // surface) as a screenshot under the given game, reusing the same encode +
    // feedback path as capture(). No foreground gating — the caller owns the
    // pixels already.
    void saveImage(const QImage& img, const QString& gameName,
                   const QString& executablePath = QString());
    void prepareForUpdate();
    void cancelUpdatePreparation();

signals:
    void grabbed();                        // pixels are in hand — play shutter NOW
    void captured(const QString& filePath, const QString& gameName,
                  const QString& executablePath);
    void skipped(const QString& reason);   // gate said "not in a game"
    void failed(const QString& reason);    // grab or save error
    void updateReady();

private:
    QImage grabRect(void* hwnd, int x, int y, int w, int h) const;
    // Shared tail of capture()/saveImage(): read format/quality on this thread,
    // then encode + write on a pool thread and emit captured()/failed().
    void encodeAndSave(const QImage& img, const QString& gameName,
                       const QString& executablePath);

    ConfigManager* m_config;
    CaptureLocations* m_locations;
    std::atomic_int m_pendingWrites{0};
    std::atomic_bool m_updatePreparing{false};
};
