#include "overlay/OverlayManager.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QScreen>
#include <QDebug>

#include <windows.h>

// Force-foreground helper. Qt's requestActivate() ends in SetForegroundWindow,
// which Win32 routinely rejects with a foreground-lock denial when our thread
// isn't the active input one (e.g. the game owns focus). AttachThreadInput
// briefly marries our input queue to the current foreground's and the target's,
// which makes SetForegroundWindow behave as if the user clicked the target —
// bypassing the lock. Symmetric: used both when stealing focus for the overlay
// (show) and when handing focus back to the game (hide).
namespace {
bool forceForeground(HWND target) noexcept
{
    if (!target)
        return false;
    if (GetForegroundWindow() == target)
        return true;

    const DWORD myThread  = GetCurrentThreadId();
    const HWND  currentFg = GetForegroundWindow();
    const DWORD fgThread  = currentFg ? GetWindowThreadProcessId(currentFg, nullptr) : 0;
    const DWORD tgtThread = GetWindowThreadProcessId(target, nullptr);

    const bool a1 = (fgThread && fgThread != myThread)
                    ? AttachThreadInput(myThread, fgThread, TRUE) : false;
    const bool a2 = (tgtThread != myThread && tgtThread != fgThread)
                    ? AttachThreadInput(myThread, tgtThread, TRUE) : false;

    bool ok = SetForegroundWindow(target) != 0;
    BringWindowToTop(target);
    SetWindowPos(target, HWND_TOP, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

    if (a2) AttachThreadInput(myThread, tgtThread, FALSE);
    if (a1) AttachThreadInput(myThread, fgThread, FALSE);

    return ok;
}
}  // namespace

OverlayManager::OverlayManager(QQmlApplicationEngine* engine, QObject* parent)
    : QObject(parent)
    , m_engine(engine)
{
}

bool OverlayManager::isVisible() const
{
    return m_window && m_window->isVisible();
}

bool OverlayManager::ensureLoaded()
{
    if (m_window)
        return true;
    m_engine->loadFromModule("GameHQ", "OverlayWindow");
    const auto roots = m_engine->rootObjects();
    for (QObject* root : roots) {
        if (root->objectName() == QLatin1String("gamehqOverlay")) {
            m_window = qobject_cast<QQuickWindow*>(root);
            break;
        }
    }
    if (!m_window) {
        qCritical() << "Overlay: failed to load OverlayWindow.qml";
        return false;
    }
    m_window->setFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint
                       | Qt::Tool);
    return true;
}

void OverlayManager::toggle()
{
    if (isVisible())
        hide();
    else
        show();
}

void OverlayManager::show()
{
    if (!ensureLoaded() || isVisible())
        return;

    // Remember the app that owns the screen right now (usually the game).
    m_previousForeground = GetForegroundWindow();
    emit aboutToShow();

    // Cover the screen the previous app is on; fall back to primary.
    QScreen* target = QGuiApplication::primaryScreen();
    if (m_previousForeground) {
        const HMONITOR monitor = MonitorFromWindow(
            static_cast<HWND>(m_previousForeground), MONITOR_DEFAULTTOPRIMARY);
        const auto screens = QGuiApplication::screens();
        for (QScreen* screen : screens) {
            if (MonitorFromPoint(POINT{ screen->geometry().center().x(),
                                        screen->geometry().center().y() },
                                 MONITOR_DEFAULTTONULL) == monitor) {
                target = screen;
                break;
            }
        }
    }
    m_window->setGeometry(target->geometry());
    m_window->show();
    m_window->raise();
    m_window->requestActivate();

    // Stage-1 input isolation: aggressively take the OS foreground so the
    // game underneath stops being the foreground window. Many games then
    // stop polling the pad (esp. borderless/windowed ones); those that
    // keep reading XInput/RawInput in the background still react — that
    // path needs the future HidHide integration (see docs/overlay.md).
    const HWND overlayHwnd = reinterpret_cast<HWND>(m_window->winId());
    const HWND fgBefore = GetForegroundWindow();
    const bool grabbed = forceForeground(overlayHwnd);
    const HWND fgAfter = GetForegroundWindow();
    qInfo() << "Overlay: shown over" << m_previousForeground
            << "| fgBefore=" << fgBefore
            << "| forceForeground=" << (grabbed ? "ok" : "FAILED")
            << "| fgAfter=" << fgAfter
            << "(fgAfter==overlay:" << (fgAfter == overlayHwnd) << ")";
    emit visibleChanged();
}

void OverlayManager::hide()
{
    if (!isVisible())
        return;
    m_window->hide();
    // Hand focus back to the game (docs/overlay.md). The original plain
    // SetForegroundWindow often got denied by foreground-lock; route it
    // through the same AttachThreadInput bypass the show() path uses.
    if (m_previousForeground) {
        const HWND prev = static_cast<HWND>(m_previousForeground);
        const HWND fgBefore = GetForegroundWindow();
        const bool ok = forceForeground(prev);
        const HWND fgAfter = GetForegroundWindow();
        qInfo() << "Overlay: focus restore | prev=" << prev
                << "| fgBefore=" << fgBefore
                << "| forceForeground=" << (ok ? "ok" : "FAILED")
                << "| fgAfter=" << fgAfter
                << "(fgAfter==prev:" << (fgAfter == prev) << ")";
        m_previousForeground = nullptr;
    } else {
        qInfo() << "Overlay: hidden, no previous foreground to restore";
    }
    emit visibleChanged();
}
