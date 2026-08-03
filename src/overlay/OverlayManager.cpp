#include "overlay/OverlayManager.h"

#include "overlay/ForegroundAcquirer.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QScreen>
#include <QDebug>

#include <windows.h>

// The AttachThreadInput force-foreground dance lives behind the ForegroundApi
// seam (overlay/ForegroundApi.cpp); ForegroundAcquirer adds the verify +
// bounded-retry policy and reports the honest result. This file only decides
// WHEN to acquire and what the result means for overlay state.
namespace {
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

    m_acquirer = new ForegroundAcquirer(this);
    connect(m_acquirer, &ForegroundAcquirer::finished, this,
            [this](const QString& phase, void*, bool acquired, int) {
        // Only the show phase feeds the isolation state; a failed focus
        // hand-back on hide is the shell's business, not an overlay claim.
        if (phase != QLatin1String("overlay show"))
            return;
        if (!acquired)
            qWarning() << "Overlay: open WITHOUT foreground — the game may still"
                          " receive controller input";
        if (m_foregroundAcquired != acquired) {
            m_foregroundAcquired = acquired;
            emit foregroundAcquiredChanged();
        }
    });
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

    // Stage-1 input isolation: take the OS foreground so the game underneath
    // stops being the foreground window. Many games then stop polling the pad
    // (esp. borderless/windowed ones); those that keep reading XInput/RawInput
    // in the background still react — that path needs the future Exclusive
    // Controller Mode (see docs/overlay.md). The acquirer verifies the result,
    // retries at most twice, and reports the truth into foregroundAcquired.
    const HWND overlayHwnd = reinterpret_cast<HWND>(m_window->winId());
    qInfo() << "Overlay: shown over" << m_previousForeground
            << "| overlay hwnd=" << overlayHwnd;
    m_acquirer->acquire(overlayHwnd, QStringLiteral("overlay show"));
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
        qInfo() << "Overlay: hidden, restoring focus to" << prev;
        m_acquirer->acquire(prev, QStringLiteral("overlay hide"));
        m_previousForeground = nullptr;
    } else {
        qInfo() << "Overlay: hidden, no previous foreground to restore";
    }
    // A closed overlay makes no isolation claim; clear any stale warning.
    if (!m_foregroundAcquired) {
        m_foregroundAcquired = true;
        emit foregroundAcquiredChanged();
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
