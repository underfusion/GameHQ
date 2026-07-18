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

// Only one OverlayManager exists per process; the WinEvent callback is a
// free function (Win32 API requirement) so it reaches the instance here.
OverlayManager* g_overlayManagerInstance = nullptr;

// Fires for EVERY OS foreground-window change, system-wide — this is how we
// catch the Windows key (opens Start), Alt-Tab / the task switcher, and
// clicking another app, without hard-coding any specific key combo. When the
// new foreground window isn't the overlay itself and the overlay is showing,
// treat it as "something stole focus" and close the overlay.
void CALLBACK onForegroundEvent(HWINEVENTHOOK, DWORD event, HWND hwnd,
                                 LONG idObject, LONG idChild, DWORD, DWORD)
{
    if (event != EVENT_SYSTEM_FOREGROUND || idObject != OBJID_WINDOW || idChild != CHILDID_SELF)
        return;
    if (g_overlayManagerInstance)
        g_overlayManagerInstance->onForegroundWindowChanged(hwnd);
}
}  // namespace

OverlayManager::OverlayManager(QQmlApplicationEngine* engine, QObject* parent)
    : QObject(parent)
    , m_engine(engine)
{
    g_overlayManagerInstance = this;
    // WINEVENT_OUTOFCONTEXT: delivered via this thread's message queue, no
    // DLL injection into other processes needed — safe for the "never inject
    // into game processes" rule (docs/overlay.md).
    m_focusHook = SetWinEventHook(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND,
                                   nullptr, onForegroundEvent, 0, 0, WINEVENT_OUTOFCONTEXT);
    if (!m_focusHook)
        qWarning() << "Overlay: SetWinEventHook failed — auto-hide on focus loss disabled";
}

OverlayManager::~OverlayManager()
{
    if (m_focusHook)
        UnhookWinEvent(static_cast<HWINEVENTHOOK>(m_focusHook));
    if (g_overlayManagerInstance == this)
        g_overlayManagerInstance = nullptr;
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
    hideInternal(/*restoreFocus=*/true);
}

void OverlayManager::hideInternal(bool restoreFocus)
{
    if (!isVisible())
        return;
    m_window->hide();
    // Hand focus back to the game (docs/overlay.md). The original plain
    // SetForegroundWindow often got denied by foreground-lock; route it
    // through the same AttachThreadInput bypass the show() path uses.
    // Skipped when the OS itself just moved focus elsewhere (Win key /
    // Alt-Tab / task switch) — forcing it back to the game would fight
    // whatever the user just opened.
    if (!restoreFocus) {
        m_previousForeground = nullptr;
        qInfo() << "Overlay: auto-hidden on focus loss, not restoring focus to the game";
    } else if (m_previousForeground) {
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

void OverlayManager::onForegroundWindowChanged(void* newForeground)
{
    if (!isVisible())
        return;
    const HWND overlayHwnd = reinterpret_cast<HWND>(m_window->winId());
    if (static_cast<HWND>(newForeground) == overlayHwnd)
        return;  // the overlay grabbing its own foreground during show() — expected, not a focus loss

    qInfo() << "Overlay: foreground moved away to" << newForeground
            << "(Windows key / Alt-Tab / task switch / other app) — auto-hiding";
    hideInternal(/*restoreFocus=*/false);
}
