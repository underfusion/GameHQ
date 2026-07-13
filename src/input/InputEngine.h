#pragma once
#include "input/ActionCatalog.h"
#include <QObject>
#include <QString>
#include <functional>
#include <memory>
#include <vector>

class ConfigManager;
class CaptureDatabase;
class Gamepad;
class DualSenseDevice;
class XInputDevice;
class WinMMDevice;
class BindingRuntime;
class BindingEditorModel;
class HotkeyManager;
class MouseHookDevice;
class QTimer;

// Owns the controller backends (Sony Raw Input, XInput, WinMM) + Share
// tap/hold detector and maps buttons onto GameHQ actions
// (docs/controller-input.md). Global triggers (Share tap/hold, PS toggle)
// fire regardless of overlay state; when the overlay is visible the face
// buttons / d-pad drive its navigation instead of leaking out as actions.
//
// Exactly ONE backend is "active" at a time (priority Sony > XInput >
// WinMM among the connected ones) and only its events are routed — the same
// physical pad is often visible through several APIs at once (DSX/ViGEm),
// and routing more than one would double every press. Switching or losing
// the active backend cancels any in-flight gesture (nav repeat, Share
// tap/hold) so nothing sticks across the transition. The Raw Input backend's
// device-topology hint triggers XInput/WinMM rescans, so pads appearing in
// those APIs are picked up event-driven instead of by hot polling.
// Exposed to QML as "input" for the Settings input-test screen.
class InputEngine : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString lastInput READ lastInput NOTIFY lastInputChanged)
    Q_PROPERTY(QString controllerStatus READ controllerStatus NOTIFY controllerStatusChanged)
    Q_PROPERTY(QObject* bindingEditor READ bindingEditor CONSTANT)
public:
    InputEngine(ConfigManager* config, CaptureDatabase* db, HotkeyManager* hotkeys,
                QObject* parent = nullptr);
    ~InputEngine() override;

    void start();   // seed default bindings + begin listening

    QString lastInput() const { return m_lastInput; }
    QString controllerStatus() const { return m_controllerStatus; }
    QObject* bindingEditor() const;

public slots:
    void setOverlayVisible(bool visible);
    // Desktop gallery window's OS focus state (Main.qml binds this to
    // window.active). Pad navigation only reaches the desktop window while
    // it's genuinely the foreground window — same "never steal the pad from
    // a game" rule the overlay already follows.
    void setDesktopFocused(bool focused);
    void setPlaybackActive(bool active);
    void reloadBindings();
    Q_INVOKABLE bool handleKeyPressed(int key, int modifiers, bool autoRepeat = false);
    Q_INVOKABLE bool handleKeyReleased(int key, int modifiers);

signals:
    // Global actions (0.4/0.5 wire these to real capture; for now sound + log).
    void screenshotRequested();
    void replayRequested();
    void bufferToggleRequested();
    void overlayToggleRequested();
    // Circle: consumed entirely in QML now (OverlayWindow.qml) — pops the
    // action menu / sidebar focus first, only closes the overlay at the
    // root level. Kept named "HideRequested" since Esc still means "close".
    void overlayHideRequested();
    // Overlay navigation (consumed by OverlayWindow.qml while it is open).
    void overlayNavigate(int direction);           // D-pad left/right: -1/+1
    void overlayNavigateVertical(int direction);   // D-pad up/down: -1/+1
    void overlayConfirm();
    void overlayFavorite();
    void overlayMenu();             // Square: open/close the per-capture action menu
    void overlaySidebarToggle();    // Options: enter/exit the sidebar
    void overlayGameStep(int direction);   // L1/R1: quick game switch, -1/+1

    // Desktop gallery navigation (consumed by Main.qml while it is the
    // foreground window and the overlay is closed).
    void desktopNavigate(int direction);           // D-pad left/right: -1/+1
    void desktopNavigateVertical(int direction);   // D-pad up/down: -1/+1
    void desktopConfirm();          // Cross: open the lightbox
    void desktopBack();             // Circle: close menu, else close lightbox
    void desktopFavorite();         // Triangle
    void desktopMenu();             // Square: open/close the per-capture action menu
    void desktopTabStep(int direction);   // L1/R1: switch panel focus (sidebar ↔ grid), -1/+1
    void playbackPlayPause();
    void playbackSeek(int direction);

    void lastInputChanged();
    void controllerStatusChanged();

private:
    void onControlPressed(const QString& controlId, int family,
                          const QString& backend, const QString& fingerprint,
                          const QString& displayName, int legacyButton);
    void onControlReleased(const QString& controlId, int family,
                           const QString& backend, const QString& fingerprint,
                           const QString& displayName, int legacyButton);
    void attachGamepad(std::unique_ptr<Gamepad> pad, const QString& displayName);
    void updateActiveBackend();
    QString backendDisplayName(const Gamepad* pad) const;
    void setLastInput(const QString& text);
    void setControllerStatus(const QString& text);
    bool desktopCanReceiveInput() const;
    bool anyBackendConnected() const;
    ActionCatalog::Scope primaryScope() const;
    ActionCatalog::Scope fallbackScope() const;
    void dispatchAction(const QString& actionId, const QString& triggerCode = {});

    // Hold-to-repeat for navigation buttons (D-pad ↑↓←→, L1, R1). The pad
    // only delivers press edges to QML, so the repeat lives here: the signal
    // fires once on press, then — if the button stays held — again 220 ms
    // later (no initial delay), then accelerating until release. Same UX as
    // keyboard auto-repeat in QML (see components/NavRepeat.qml).
    //   startInterval: 220 ms between the first auto-repeats (also the
    //                  tap-release guard — release before this fires once)
    //   acceleration: each tick multiplies the interval by this (0.77 = 0.88²,
    //                 2x the shrink-per-tick of the original 0.88)
    //   minInterval: floor at 70 ms (≈14 steps/sec)
    void startNavRepeat(const QString& triggerCode, int direction,
                        std::function<void(int)> emitter);
    void stopNavRepeat();

    ConfigManager* m_config;
    CaptureDatabase* m_db;
    HotkeyManager* m_hotkeys;
    std::unique_ptr<BindingRuntime> m_runtime;
    std::unique_ptr<BindingEditorModel> m_bindingEditor;
    std::unique_ptr<MouseHookDevice> m_mouse;
    std::vector<std::unique_ptr<Gamepad>> m_pads;
    DualSenseDevice* m_sonyPad = nullptr;
    XInputDevice* m_xinputPad = nullptr;
    WinMMDevice* m_winmmPad = nullptr;
    Gamepad* m_activeBackend = nullptr;   // the one backend whose events route
    bool m_sonyConnected = false;
    bool m_xinputConnected = false;
    bool m_winmmConnected = false;
    bool m_overlayVisible = false;
    bool m_desktopFocused = false;
    bool m_playbackActive = false;
    QString m_lastInput;
    QString m_controllerStatus;

    QTimer* m_repeatTick = nullptr;      // accelerating repeat timer
    QString m_repeatTrigger;             // canonical control currently held
    int m_repeatDirection = 0;
    std::function<void(int)> m_repeatEmitter;
};
class Gamepad;
