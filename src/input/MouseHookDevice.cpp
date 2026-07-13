#include "input/MouseHookDevice.h"

#include <QDebug>

const QString MouseHookDevice::ButtonBack    = QStringLiteral("mouse.button4");
const QString MouseHookDevice::ButtonForward = QStringLiteral("mouse.button5");
const QString MouseHookDevice::ButtonMiddle  = QStringLiteral("mouse.middle");

MouseHookDevice* MouseHookDevice::s_instance = nullptr;

MouseHookDevice::MouseHookDevice(QObject* parent)
    : QObject(parent)
{
}

MouseHookDevice::~MouseHookDevice()
{
    if (m_hook) {
        UnhookWindowsHookEx(static_cast<HHOOK>(m_hook));
        if (s_instance == this)
            s_instance = nullptr;
    }
}

bool MouseHookDevice::start()
{
    if (s_instance) {
        qWarning() << "MouseHook: another instance is already active in this process";
        return false;
    }
    // Must run on a thread with a message loop (WH_MOUSE_LL delivers on the
    // installing thread) — the Qt main thread already pumps Windows
    // messages for RegisterHotKey/Raw Input, so this belongs there too.
    s_instance = this;
    m_hook = SetWindowsHookExW(WH_MOUSE_LL, &MouseHookDevice::lowLevelProc, nullptr, 0);
    if (!m_hook) {
        qWarning() << "MouseHook: SetWindowsHookEx failed, error" << GetLastError();
        s_instance = nullptr;
        return false;
    }
    qInfo() << "MouseHook: extra mouse buttons (Back/Forward/Middle) observed globally";
    return true;
}

LRESULT CALLBACK MouseHookDevice::lowLevelProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION && s_instance)
        s_instance->handleEvent(wParam, lParam);
    // Always pass through unmodified — this hook only observes. Left/right
    // clicks are never inspected above, let alone blocked.
    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

void MouseHookDevice::handleEvent(WPARAM wParam, LPARAM lParam)
{
    switch (wParam) {
    case WM_XBUTTONDOWN:
    case WM_XBUTTONUP: {
        const auto* info = reinterpret_cast<const MSLLHOOKSTRUCT*>(lParam);
        const WORD xButton = HIWORD(info->mouseData);
        const QString& code = (xButton == XBUTTON1) ? ButtonBack : ButtonForward;
        if (wParam == WM_XBUTTONDOWN)
            emit buttonPressed(code);
        else
            emit buttonReleased(code);
        break;
    }
    case WM_MBUTTONDOWN:
        emit buttonPressed(ButtonMiddle);
        break;
    case WM_MBUTTONUP:
        emit buttonReleased(ButtonMiddle);
        break;
    default:
        break;   // WM_MOUSEMOVE and everything else — cheap no-op, never inspected further
    }
}
