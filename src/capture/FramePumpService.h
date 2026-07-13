#pragma once
#include <QObject>
#include <QThread>

class ConfigManager;
class CaptureLocations;
class QTimer;

// Milestone 0.5, Step 3 — Windows Graphics Capture frame pump (log-only, no encode).
//
// Toggled by Ctrl+Shift+R. On start it captures the current foreground game window
// (gated by capture.mode, same as the screenshot path) via a free-threaded WGC
// Direct3D11CaptureFramePool, polls TryGetNextFrame on a ~16 ms timer, reads each
// frame's ID3D11Texture2D description and logs size/format + a per-second fps count.
// No frames are encoded or kept — this substage only proves the live frame pump
// works on the MinGW/raw-ABI WGC path (docs/capture-engine.md, wgc_shims.h).
//
// All WinRT/D3D work runs on a dedicated MTA thread (FramePumpWorker): Qt's GUI
// thread is initialised as an STA, which is incompatible with RoInitialize(MTA) and
// the free-threaded frame pool. FramePumpService is the GUI-thread front-end.

// Runs on its own thread; owns all WGC/D3D state (opaque Pipeline, defined in the .cpp).
class FramePumpWorker : public QObject
{
    Q_OBJECT
public:
    explicit FramePumpWorker(QObject* parent = nullptr);
    ~FramePumpWorker() override;

public slots:
    void onThreadStarted();          // RoInitialize(MTA) on this worker thread
    void onThreadFinished();         // ensure stopped + RoUninitialize
    void startPump(qulonglong hwnd, unsigned long pid, int encodeWidth, int encodeHeight,
                   int fps, int bitrateMbps, int segmentSeconds, int lengthSeconds,
                   const QString& gameName, const QString& executablePath,
                   bool audioEnabled); // build pipeline + start polling/encoding
    void stopPump();                 // stop polling + tear down the pipeline
    void saveReplayOnWorker(const QString& clipsBaseRoot);

private slots:
    void poll();                     // one TryGetNextFrame tick

signals:
    void failed(const QString& reason);
    // Fired the instant the ring is frozen (~1 s into the hold), before the
    // slower remux — thumbnailPath is a preview grabbed from the freshest
    // completed segment so the "saved" toast can show it right away.
    void clipSaving(const QString& gameName, const QString& thumbnailPath,
                    const QString& executablePath);
    void clipSaved(const QString& clipPath, const QString& gameName,
                   const QString& thumbnailPath, const QString& executablePath);
    void clipFailed(const QString& gameName, const QString& reason);

private:
    struct Pipeline;                 // all WGC/D3D pointers + timer + fps state (.cpp)
    void teardown();                 // delete m_pipe (releases everything, reverse order)

    Pipeline* m_pipe = nullptr;
    bool m_apartmentReady = false;
    bool m_exportBusy = false;       // one async clip export at a time
};

// GUI-thread owner: constructs the worker on a dedicated thread and relays toggle().
class FramePumpService : public QObject
{
    Q_OBJECT
public:
    explicit FramePumpService(ConfigManager* config, CaptureLocations* locations,
                              QObject* parent = nullptr);
    ~FramePumpService() override;

public slots:
    void toggle();                   // Ctrl+Shift+R: master on/off for always-on recording
    void saveReplay();               // Share-hold: save the last N seconds as one clip
    // Replay settings changed (fps/resolution/length): disarm a running
    // buffer so the auto-arm tick re-arms it with the new parameters.
    void restartBuffer();

signals:
    void failed(const QString& reason);
    void clipSaving(const QString& gameName, const QString& thumbnailPath,
                    const QString& executablePath);   // ring frozen - instant feedback
    void clipSaved(const QString& clipPath, const QString& gameName,
                   const QString& thumbnailPath, const QString& executablePath);
    void clipFailed(const QString& gameName, const QString& reason);
    void foregroundGameDetected(const QString& gameName, const QString& executablePath);
    // Rolling buffer armed/disarmed — drives the Settings "buffer state" row.
    void recordingStateChanged(bool active, const QString& gameName);

private slots:
    void autoTick();                 // poll the foreground game → arm/disarm the buffer

private:
    void startBuffer();              // arm on the current foreground game (if it is a game)
    void stopBuffer();               // disarm

    ConfigManager* m_config = nullptr;
    CaptureLocations* m_locations = nullptr;
    QThread m_thread;
    FramePumpWorker* m_worker = nullptr;   // lives on m_thread; deleted after it stops
    bool m_running = false;

    // Always-on auto-arm (replay.auto): while enabled, the buffer records whenever a
    // game is foreground (per capture.mode) — no manual arming needed.
    QTimer* m_autoTimer = nullptr;
    bool m_autoEnabled = true;
    int m_noGameTicks = 0;                 // grace period before auto-disarm
};
