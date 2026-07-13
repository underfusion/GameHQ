#pragma once
#include <QObject>
#include <QString>

#include <windows.h>

// Global source for extra (non-primary) mouse buttons only. Installs a
// low-level Windows mouse hook (WH_MOUSE_LL)
// that OBSERVES XButton1/XButton2/middle-click and unconditionally passes
// every event through (CallNextHookEx) — it never swallows or injects
// input. Left/right clicks are never read here at all, so ordinary
// clicking anywhere (games included) is completely unaffected.
class MouseHookDevice : public QObject
{
    Q_OBJECT
public:
    explicit MouseHookDevice(QObject* parent = nullptr);
    ~MouseHookDevice() override;

    // Canonical codes for the buttons this device can report.
    static const QString ButtonBack;      // "mouse.button4" (XBUTTON1 / Back)
    static const QString ButtonForward;   // "mouse.button5" (XBUTTON2 / Forward)
    static const QString ButtonMiddle;    // "mouse.middle"

    // Installs the hook. Returns false only on a hard setup failure (e.g.
    // another MouseHookDevice already active in this process); "hook not
    // needed yet" is for the caller to decide, not this class.
    bool start();

signals:
    void buttonPressed(const QString& code);
    void buttonReleased(const QString& code);

private:
    static LRESULT CALLBACK lowLevelProc(int nCode, WPARAM wParam, LPARAM lParam);
    void handleEvent(WPARAM wParam, LPARAM lParam);

    void* m_hook = nullptr;
    // WH_MOUSE_LL requires a plain function pointer, so the single active
    // instance is reached through this — SetWindowsHookEx allows only one
    // hook chain per thread anyway, so one process-wide instance is the
    // natural limit, not an artificial one.
    static MouseHookDevice* s_instance;
};
