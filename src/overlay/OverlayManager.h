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

    bool isVisible() const;

    Q_INVOKABLE void toggle();
    Q_INVOKABLE void show();
    Q_INVOKABLE void hide();

signals:
    void aboutToShow();
    void visibleChanged();

private:
    bool ensureLoaded();

    QQmlApplicationEngine* m_engine;
    QQuickWindow* m_window = nullptr;
    void* m_previousForeground = nullptr;   // HWND of the game/app under us
};
