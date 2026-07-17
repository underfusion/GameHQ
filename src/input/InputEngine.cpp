#include "input/InputEngine.h"

#include "config/ConfigKeys.h"
#include "config/ConfigManager.h"
#include "input/BindingRuntime.h"
#include "input/BindingEditorModel.h"
#include "input/DualSenseDevice.h"
#include "input/Gamepad.h"
#include "input/HotkeyManager.h"
#include "input/MouseHookDevice.h"
#include "input/WinMMDevice.h"
#include "input/XInputDevice.h"
#include "storage/CaptureDatabase.h"

#include <QDebug>
#include <QKeySequence>
#include <QSet>
#include <QTimer>

#include <windows.h>

namespace {
QString keyboardTrigger(int key, int modifiers)
{
    if (key == Qt::Key_Enter)
        return QStringLiteral("Enter");
    return QKeySequence(key | modifiers).toString(QKeySequence::PortableText);
}
}

InputEngine::InputEngine(ConfigManager* config, CaptureDatabase* db,
                         HotkeyManager* hotkeys, QObject* parent)
    : QObject(parent)
    , m_config(config)
    , m_db(db)
    , m_hotkeys(hotkeys)
    , m_runtime(std::make_unique<BindingRuntime>(db))
    , m_bindingEditor(std::make_unique<BindingEditorModel>(
          db, m_runtime.get(), [this] { reloadBindings(); }))
    , m_mouse(std::make_unique<MouseHookDevice>())
    , m_lastInput(QStringLiteral("Connect a controller and press a button..."))
    , m_controllerStatus(QStringLiteral("No controller detected"))
{
    m_runtime->setDefaultHoldMs(
        m_config->value(ConfigKeys::InputShareHoldMs, 2000).toInt());
    connect(m_config, &ConfigManager::valueChanged, this,
            [this](const QString& key, const QVariant& value) {
                if (key != ConfigKeys::InputShareHoldMs)
                    return;
                const int threshold = qBound(250, value.toInt(), 10000);
                m_runtime->setDefaultHoldMs(threshold);
                reloadBindings();
                qInfo() << "Input: capture-button hold threshold" << threshold << "ms";
            });

    connect(m_runtime.get(), &BindingRuntime::actionTriggered,
            this, &InputEngine::dispatchAction);
    if (m_hotkeys) {
        connect(m_hotkeys, &HotkeyManager::hotkeyTriggered, this,
                [this](const QString& actionId) { dispatchAction(actionId); });
    }
    connect(m_mouse.get(), &MouseHookDevice::buttonPressed, this,
            [this](const QString& code) {
                QString label = code;
                if (code == MouseHookDevice::ButtonBack) label = QStringLiteral("Mouse Back");
                else if (code == MouseHookDevice::ButtonForward) label = QStringLiteral("Mouse Forward");
                else if (code == MouseHookDevice::ButtonMiddle) label = QStringLiteral("Middle Mouse");
                if (m_bindingEditor->captureInput(QStringLiteral("mouse"), code, label))
                    return;
                m_runtime->press(QStringLiteral("mouse"), {}, code,
                                 primaryScope(), fallbackScope());
            });
    connect(m_mouse.get(), &MouseHookDevice::buttonReleased, this,
            [this](const QString& code) {
                m_runtime->release(QStringLiteral("mouse"), {}, code);
                if (code == m_repeatTrigger)
                    stopNavRepeat();
            });

    auto sonyPad = std::make_unique<DualSenseDevice>();
    m_sonyPad = sonyPad.get();
    attachGamepad(std::move(sonyPad), QStringLiteral("Sony controller"));

    auto xinputPad = std::make_unique<XInputDevice>();
    m_xinputPad = xinputPad.get();
    attachGamepad(std::move(xinputPad), QStringLiteral("XInput controller"));

    auto winmmPad = std::make_unique<WinMMDevice>();
    m_winmmPad = winmmPad.get();
    attachGamepad(std::move(winmmPad), QStringLiteral("WinMM joystick"));

    // The Raw Input backend sees every HID arrival/removal (debounced),
    // including XInput and DirectInput devices. Use it as the hot-plug
    // trigger for the polling backends so they detect new pads immediately
    // without continuously probing empty slots.
    connect(m_sonyPad, &DualSenseDevice::deviceTopologyChanged, this, [this] {
        qInfo() << "Input: device topology changed — rescanning fallback backends";
        m_xinputPad->rescan();
        m_winmmPad->rescan();
    });

    // Hold-to-repeat tick for pad navigation (D-pad + L1/R1). Built once and
    // reused for whichever direction is currently held — only one pad button
    // is physically held at a time on a single d-pad/stick face.
    // No separate "initial delay" timer: startNavRepeat starts the tick
    // immediately at 220 ms (which also serves as the tap-release guard),
    // then each tick accelerates toward the 70 ms floor.
    m_repeatTick = new QTimer(this);
    m_repeatTick->setTimerType(Qt::PreciseTimer);
    m_repeatTick->setInterval(220);
    connect(m_repeatTick, &QTimer::timeout, this, [this] {
        if (m_repeatEmitter)
            m_repeatEmitter(m_repeatDirection);
        // Accelerate: shrink the interval toward the floor.
        // 0.77 = 0.88² — twice the shrink-per-tick of the old 0.88, so the
        // ramp reaches its floor in half as many ticks (2x faster ramp).
        m_repeatTick->setInterval(
            qMax(70, int(m_repeatTick->interval() * 0.77)));
    });
}

InputEngine::~InputEngine() = default;

QObject* InputEngine::bindingEditor() const
{
    return m_bindingEditor.get();
}

void InputEngine::start()
{
    if (m_db)
        m_db->seedDefaultBindings();
    reloadBindings();
    m_mouse->start();
    for (const auto& pad : m_pads)
        pad->start();
}

void InputEngine::reloadBindings()
{
    m_runtime->reload();
    if (!m_hotkeys)
        return;

    // Apply replacements before clearing removed actions, preserving the live
    // shortcut if Windows rejects a newly requested chord.
    QSet<QString> desiredBindings;
    for (const auto& binding : m_runtime->effectiveBindings(QStringLiteral("keyboard"))) {
        const auto* action = ActionCatalog::find(binding.actionId);
        if (!action || action->scope != ActionCatalog::Scope::Global
            || binding.activation != QLatin1String("press"))
            continue;
        desiredBindings.insert(binding.actionId + QLatin1Char('#')
                               + QString::number(binding.slot));
        const auto chord = HotkeyManager::parseChord(binding.triggerCode);
        if (!chord.valid) {
            qWarning().noquote() << QStringLiteral("Hotkey: ignored invalid saved binding %1: %2")
                                      .arg(binding.triggerCode, chord.rejectionReason);
            continue;
        }
        m_hotkeys->applyBindingSlot(binding.actionId, binding.slot,
                                    chord.modifiers, chord.vk);
    }
    for (const auto& action : ActionCatalog::all()) {
        if (action.scope != ActionCatalog::Scope::Global)
            continue;
        for (int slot = 1; slot <= 2; ++slot) {
            const QString key = action.id + QLatin1Char('#') + QString::number(slot);
            if (!desiredBindings.contains(key))
                m_hotkeys->clearBindingSlot(action.id, slot);
        }
    }
}

bool InputEngine::handleKeyPressed(int key, int modifiers, bool autoRepeat)
{
    const QString trigger = keyboardTrigger(key, modifiers);
    if (trigger.isEmpty())
        return false;
    if (!autoRepeat && m_bindingEditor->captureInput(
            QStringLiteral("keyboard"), trigger, trigger))
        return true;
    if (autoRepeat) {
        // BindingRuntime owns a consistent accelerating repeat curve.
        const auto bindings = m_runtime->effectiveBindings(QStringLiteral("keyboard"));
        for (const auto& binding : bindings) {
            if (binding.triggerCode == trigger)
                return true;
        }
        return false;
    }
    return m_runtime->press(QStringLiteral("keyboard"), {}, trigger,
                            primaryScope(), fallbackScope());
}

bool InputEngine::handleKeyReleased(int key, int modifiers)
{
    const QString trigger = keyboardTrigger(key, modifiers);
    const bool handled = m_runtime->release(QStringLiteral("keyboard"), {}, trigger);
    if (trigger == m_repeatTrigger)
        stopNavRepeat();
    return handled;
}

void InputEngine::attachGamepad(std::unique_ptr<Gamepad> pad, const QString& displayName)
{
    Gamepad* raw = pad.get();
    connect(raw, &Gamepad::controlPressed, this, &InputEngine::onControlPressed);
    connect(raw, &Gamepad::controlReleased, this, &InputEngine::onControlReleased);
    connect(raw, &Gamepad::connected, this, [this, raw, displayName](bool c) {
        if (raw == m_sonyPad)
            m_sonyConnected = c;
        else if (raw == m_xinputPad)
            m_xinputConnected = c;
        else if (raw == m_winmmPad)
            m_winmmConnected = c;

        updateActiveBackend();
        if (!c && !anyBackendConnected())
            setLastInput(displayName + QStringLiteral(" disconnected"));
    });
    m_pads.push_back(std::move(pad));
}

// Pick the one backend whose events route to actions: Sony > XInput > WinMM
// among the connected ones. The same physical pad is often visible through
// several APIs at once (DSX/ViGEm), so routing more than one would double
// every press. Any change of the active source cancels in-flight gestures —
// a held nav repeat or a half-finished Share tap/hold must not survive into
// (or leak out of) a backend switch.
void InputEngine::updateActiveBackend()
{
    Gamepad* pick = nullptr;
    if (m_sonyConnected)
        pick = m_sonyPad;
    else if (m_xinputConnected)
        pick = m_xinputPad;
    else if (m_winmmConnected)
        pick = m_winmmPad;

    if (pick == m_activeBackend)
        return;

    stopNavRepeat();
    m_runtime->cancelAll();
    m_activeBackend = pick;

    if (pick) {
        m_bindingEditor->setControllerProfile(pick->profile());
        const QString name = backendDisplayName(pick);
        qInfo() << "Input: active controller backend →" << name;
        setControllerStatus(name + QStringLiteral(" connected"));
        setLastInput(name + QStringLiteral(" connected"));
    } else {
        m_bindingEditor->setControllerProfile({});
        qInfo() << "Input: no controller backend connected";
        setControllerStatus(QStringLiteral("No controller detected"));
    }
}

QString InputEngine::backendDisplayName(const Gamepad* pad) const
{
    if (pad == m_sonyPad)
        return QStringLiteral("Sony controller");
    if (pad == m_xinputPad)
        return QStringLiteral("XInput controller");
    if (pad == m_winmmPad)
        return QStringLiteral("WinMM joystick");
    return QStringLiteral("Controller");
}

void InputEngine::setOverlayVisible(bool visible)
{
    if (m_overlayVisible == visible)
        return;
    m_overlayVisible = visible;
    if (!visible)
        m_playbackActive = false;
    // A held navigation button shouldn't keep firing into the window we just
    // left — stop any in-flight repeat when the focus context switches.
    stopNavRepeat();
    m_runtime->cancelAll();
    qInfo() << "Input: overlay capture"
            << (visible ? "active — routing controller to overlay only"
                        : "inactive — global triggers only");
}

void InputEngine::setDesktopFocused(bool focused)
{
    m_desktopFocused = focused;
    if (!focused)
        stopNavRepeat();
}

void InputEngine::setPlaybackActive(bool active)
{
    if (m_playbackActive == active)
        return;
    m_playbackActive = active;
    stopNavRepeat();
    m_runtime->cancelAll();
}

void InputEngine::onControlPressed(const QString& controlId, int family,
                                   const QString&, const QString& fingerprint,
                                   const QString&)
{
    if (sender() != m_activeBackend)
        return;
    setLastInput(ControlId::label(controlId, static_cast<ControlId::ControllerFamily>(family))
                 + QStringLiteral(" pressed"));
    if (m_bindingEditor->captureInput(
            QStringLiteral("controller"), controlId,
            ControlId::label(controlId, static_cast<ControlId::ControllerFamily>(family))))
        return;
    m_runtime->press(QStringLiteral("controller"), fingerprint, controlId,
                     primaryScope(), fallbackScope());
}

void InputEngine::onControlReleased(const QString& controlId, int, const QString&,
                                    const QString& fingerprint, const QString&)
{
    if (sender() != m_activeBackend)
        return;
    m_runtime->release(QStringLiteral("controller"), fingerprint, controlId);
    if (controlId == m_repeatTrigger)
        stopNavRepeat();
}

void InputEngine::startNavRepeat(const QString& triggerCode, int direction,
                                 std::function<void(int)> emitter)
{
    // First, fire immediately so a quick tap still does one step.
    emitter(direction);
    // Record what we're now repeating — stopNavRepeat() uses m_repeatButton.
    m_repeatTrigger = triggerCode;
    m_repeatDirection = direction;
    m_repeatEmitter = std::move(emitter);
    // NO initial delay — kick off the accelerating tick immediately. The
    // first tick fires 220 ms after press, which doubles as the natural
    // "this was just a tap" guard: anything released before then is a
    // single step. After that, each tick accelerates toward the 70 ms floor.
    // If another direction was already repeating, this atomically replaces it.
    m_repeatTick->stop();
    m_repeatTick->setInterval(220);
    m_repeatTick->start();
}

void InputEngine::stopNavRepeat()
{
    m_repeatTick->stop();
    m_repeatTrigger.clear();
    m_repeatDirection = 0;
    m_repeatEmitter = {};
}

ActionCatalog::Scope InputEngine::primaryScope() const
{
    if (m_playbackActive)
        return ActionCatalog::Scope::Playback;
    if (m_overlayVisible)
        return ActionCatalog::Scope::Overlay;
    if (desktopCanReceiveInput())
        return ActionCatalog::Scope::Desktop;
    return ActionCatalog::Scope::Global;
}

ActionCatalog::Scope InputEngine::fallbackScope() const
{
    if (!m_playbackActive)
        return ActionCatalog::Scope::Global;
    return m_overlayVisible ? ActionCatalog::Scope::Overlay
                            : ActionCatalog::Scope::Desktop;
}

void InputEngine::dispatchAction(const QString& actionId, const QString& triggerCode)
{
    m_bindingEditor->setLastFiredAction(actionId);
    if (const auto* action = ActionCatalog::find(actionId))
        setLastInput(action->label);

    // Dispatch table: every bindable action maps to a handler. When a new action
    // is added to ActionCatalog, a matching entry must appear below — a missing
    // entry compiles and runs but is a silent no-op (caught by the qWarning at
    // the end).  The table is local-static: built once, never rebuilt.
    using Handler = void (InputEngine::*)(const QString& triggerCode);
    struct Entry { const char* actionId; Handler handler; };
    static const Entry table[] = {
        // Global
        { "global.screenshot",     &Self::handleScreenshot },
        { "global.save_replay",    &Self::handleSaveReplay },
        { "global.toggle_overlay", &Self::handleToggleOverlay },
        // Overlay
        { "overlay.navigate_left",  &Self::handleOverlayNavigateLeft },
        { "overlay.navigate_right", &Self::handleOverlayNavigateRight },
        { "overlay.navigate_up",    &Self::handleOverlayNavigateUp },
        { "overlay.navigate_down",  &Self::handleOverlayNavigateDown },
        { "overlay.confirm",        &Self::handleOverlayConfirm },
        { "overlay.back",           &Self::handleOverlayBack },
        { "overlay.favorite",       &Self::handleOverlayFavorite },
        { "overlay.menu",           &Self::handleOverlayMenu },
        { "overlay.sidebar_toggle", &Self::handleOverlaySidebarToggle },
        { "overlay.game_prev",      &Self::handleOverlayGamePrev },
        { "overlay.game_next",      &Self::handleOverlayGameNext },
        // Desktop
        { "desktop.navigate_left",  &Self::handleDesktopNavigateLeft },
        { "desktop.navigate_right", &Self::handleDesktopNavigateRight },
        { "desktop.navigate_up",    &Self::handleDesktopNavigateUp },
        { "desktop.navigate_down",  &Self::handleDesktopNavigateDown },
        { "desktop.confirm",        &Self::handleDesktopConfirm },
        { "desktop.back",           &Self::handleDesktopBack },
        { "desktop.favorite",       &Self::handleDesktopFavorite },
        { "desktop.menu",           &Self::handleDesktopMenu },
        { "desktop.tab_prev",       &Self::handleDesktopTabPrev },
        { "desktop.tab_next",       &Self::handleDesktopTabNext },
        { "desktop.settings",       &Self::handleDesktopSettings },
        { "desktop.zoom_out",       &Self::handleDesktopZoomOut },
        { "desktop.zoom_in",        &Self::handleDesktopZoomIn },
        { "desktop.bulk_toggle",    &Self::handleDesktopBulkToggle },
        // Playback
        { "playback.play_pause",    &Self::handlePlaybackPlayPause },
        { "playback.seek_back",     &Self::handlePlaybackSeekBack },
        { "playback.seek_forward",  &Self::handlePlaybackSeekForward },
        { "playback.frame_grab",     &Self::handleFrameGrab },
    };

    for (const auto& entry : table) {
        if (actionId == QLatin1String(entry.actionId)) {
            (this->*entry.handler)(triggerCode);
            return;
        }
    }

    qWarning().noquote()
        << QStringLiteral("Input: unknown action %1 — missing dispatch table entry?")
               .arg(actionId);
}

void InputEngine::setLastInput(const QString& text)
{
    if (m_lastInput == text)
        return;
    m_lastInput = text;
    qInfo() << "Input:" << text;
    emit lastInputChanged();
}

void InputEngine::setControllerStatus(const QString& text)
{
    if (m_controllerStatus == text)
        return;
    m_controllerStatus = text;
    emit controllerStatusChanged();
}

bool InputEngine::desktopCanReceiveInput() const
{
    if (!m_desktopFocused)
        return false;

    // Raw Input is registered with RIDEV_INPUTSINK, so button events arrive
    // even while a game owns focus. Treat QML's cached window.active only as
    // a hint and verify the real Win32 foreground window before emitting any
    // desktop-gallery action such as Cross -> lightbox.openAt().
    HWND foreground = GetForegroundWindow();
    if (!foreground)
        return false;

    DWORD processId = 0;
    GetWindowThreadProcessId(foreground, &processId);
    return processId == GetCurrentProcessId();
}

bool InputEngine::anyBackendConnected() const
{
    return m_sonyConnected || m_xinputConnected || m_winmmConnected;
}
