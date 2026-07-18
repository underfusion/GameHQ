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

public:
    explicit OverlayManager(QQmlApplicationEngine* engine, QObject* parent = nullptr);
    ~OverlayManager() override;

    bool isVisible() const;

    Q_INVOKABLE void toggle();
    Q_INVOKABLE void show();
    Q_INVOKABLE void hide();

    // Called from the WinEvent hook callback (see .cpp) whenever the OS
    // foreground window changes to something other than the overlay itself
    // while the overlay is visible — Win key (Start menu), Alt-Tab, the task
    // switcher, or a click on another app all land here. Public so the free
    // function callback can reach it; not meant for QML/general use.
    void onForegroundWindowChanged(void* newForeground);

signals:
    void aboutToShow();
    void visibleChanged();

private:
    bool ensureLoaded();
    void hideInternal(bool restoreFocus);

    QQmlApplicationEngine* m_engine;
    QQuickWindow* m_window = nullptr;
    void* m_previousForeground = nullptr;   // HWND of the game/app under us
    void* m_focusHook = nullptr;            // HWINEVENTHOOK, opaque here to avoid <windows.h> in the header
};
