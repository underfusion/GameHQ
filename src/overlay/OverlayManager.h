#pragma once
#include <QObject>

class QQmlApplicationEngine;
class QQuickWindow;

// In-game overlay window lifecycle (docs/overlay.md): lazy-loads
// OverlayWindow.qml, shows it frameless/topmost over the active app,
// remembers the previous foreground window and restores focus on hide.
// No injection — borderless/windowed fullscreen games only (MVP).
class OverlayManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool visible READ isVisible NOTIFY visibleChanged)
    // Honest input-isolation state: true only when the OS foreground really
    // moved to the overlay after show(). When false, the overlay is open but
    // the game underneath still owns focus — it may keep reacting to the pad,
    // and the UI must not pretend otherwise.
    Q_PROPERTY(bool foregroundAcquired READ foregroundAcquired NOTIFY foregroundAcquiredChanged)

public:
    explicit OverlayManager(QQmlApplicationEngine* engine, QObject* parent = nullptr);
    ~OverlayManager() override;

    bool isVisible() const;
    bool foregroundAcquired() const { return m_foregroundAcquired; }

    Q_INVOKABLE void toggle();
    Q_INVOKABLE void show();
    Q_INVOKABLE void hide();

    // Desktop-window summon (hold PS): the overlay gets out of the way but
    // must NOT hand focus back to the game — the desktop window is about to
    // take it. Returns the window the overlay had remembered (null when it
    // was not visible) so the caller can restore focus there later.
    void* hideForDesktopHandoff();

    // Called from the WinEvent hook callback (see .cpp) whenever the OS
    // foreground window changes to something other than the overlay itself
    // while the overlay is visible — Win key (Start menu), Alt-Tab, the task
    // switcher, or a click on another app all land here. Public so the free
    // function callback can reach it; not meant for QML/general use.
    void onForegroundWindowChanged(void* newForeground);

signals:
    void aboutToShow();
    void visibleChanged();
    void foregroundAcquiredChanged();

private:
    bool ensureLoaded();
    void hideInternal(bool restoreFocus);

    QQmlApplicationEngine* m_engine;
    QQuickWindow* m_window = nullptr;
    void* m_previousForeground = nullptr;   // HWND of the game/app under us
    void* m_focusHook = nullptr;            // HWINEVENTHOOK, opaque here to avoid <windows.h> in the header
    class ForegroundAcquirer* m_acquirer = nullptr;
    bool m_foregroundAcquired = true;
};
