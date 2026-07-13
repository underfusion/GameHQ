#pragma once
#include <QObject>
#include <QString>

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

public slots:
    void capture();   // grab per capture.mode; emits exactly one result signal

signals:
    void grabbed();                        // pixels are in hand — play shutter NOW
    void captured(const QString& filePath, const QString& gameName,
                  const QString& executablePath);
    void skipped(const QString& reason);   // gate said "not in a game"
    void failed(const QString& reason);    // grab or save error

private:
    QImage grabRect(void* hwnd, int x, int y, int w, int h) const;

    ConfigManager* m_config;
    CaptureLocations* m_locations;
};
