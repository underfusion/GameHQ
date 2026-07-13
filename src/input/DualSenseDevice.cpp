#include "input/DualSenseDevice.h"

#include <QByteArray>
#include <QDebug>
#include <QTimer>
#include <QVarLengthArray>

#include <windows.h>

namespace
{
constexpr USHORT kUsagePageGeneric = 0x01;
constexpr USHORT kUsageJoystick    = 0x04;
constexpr USHORT kUsageGamepad     = 0x05;
constexpr USHORT kUsageMultiAxis   = 0x08;
constexpr quint32 kSonyVid         = 0x054C;
constexpr quint32 kDualSensePid    = 0x0CE6;
constexpr quint32 kDualSenseEdgePid = 0x0DF2;
constexpr quint32 kVirtualDualSensePid = 0x0ECC;   // DSX/ViGEm virtual DualSense
constexpr quint32 kDualShock4V1Pid = 0x05C4;
constexpr quint32 kDualShock4V2Pid = 0x09CC;
constexpr quint32 kVirtualDs4VidA = 0x11FF;
constexpr quint32 kVirtualDs4PidA = 0x0847;
constexpr quint32 kVirtualDs4VidB = 0x3670;
constexpr quint32 kVirtualDs4PidB = 0x0902;
const wchar_t* kWndClassName       = L"GameHQRawInputWindow";

// A pad streaming reports sends one every few milliseconds even when idle.
// If the active pad has been silent this long while ANOTHER tracked pad
// shows a real button/stick change, the active role fails over to it.
constexpr qint64 kActiveSilenceMs = 1000;
// A removed/silent pad is only declared disconnected after this debounce —
// USB/Bluetooth re-enumeration makes pads blink out for a moment.
constexpr int kDisconnectDebounceMs = 1500;
// Arrival/removal bursts (Windows re-enumerates the whole HID tree at once)
// collapse into one reconciliation pass and one topology hint.
constexpr int kTopologyDebounceMs = 400;

enum ReportLayout {
    LayoutUnknown = 0,
    LayoutDualSense,
    LayoutDs4
};

DualSenseDevice* deviceFor(HWND hwnd)
{
    return reinterpret_cast<DualSenseDevice*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
}

int supportedReportLayout(quint32 vendorId, quint32 productId)
{
    if (vendorId == kSonyVid) {
        if (productId == kDualSensePid || productId == kDualSenseEdgePid
            || productId == kVirtualDualSensePid)
            return LayoutDualSense;
        if (productId == kDualShock4V1Pid || productId == kDualShock4V2Pid)
            return LayoutDs4;
    }

    if ((vendorId == kVirtualDs4VidA && productId == kVirtualDs4PidA)
        || (vendorId == kVirtualDs4VidB && productId == kVirtualDs4PidB))
        return LayoutDs4;

    return LayoutUnknown;
}

bool isGamepadUsage(const RID_DEVICE_INFO_HID& hid)
{
    return hid.usUsagePage == kUsagePageGeneric
        && (hid.usUsage == kUsageGamepad
            || hid.usUsage == kUsageJoystick
            || hid.usUsage == kUsageMultiAxis);
}

const char* padName(int layout)
{
    return layout == LayoutDs4 ? "DS4-compatible" : "DualSense";
}

QString devicePath(HANDLE handle)
{
    UINT chars = 0;
    if (GetRawInputDeviceInfoW(handle, RIDI_DEVICENAME, nullptr, &chars) != 0 || chars == 0)
        return {};
    QVarLengthArray<wchar_t, 256> buf(static_cast<int>(chars) + 1);
    if (GetRawInputDeviceInfoW(handle, RIDI_DEVICENAME, buf.data(), &chars)
            == static_cast<UINT>(-1))
        return {};
    buf[buf.size() - 1] = 0;
    return QString::fromWCharArray(buf.data());
}

LRESULT CALLBACK rawInputWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_INPUT:
        if (auto* dev = deviceFor(hwnd))
            dev->onRawInput(reinterpret_cast<void*>(lParam));
        return 0;
    case WM_INPUT_DEVICE_CHANGE:
        if (auto* dev = deviceFor(hwnd))
            dev->onDeviceChange(wParam == GIDC_ARRIVAL, reinterpret_cast<void*>(lParam));
        return 0;
    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}
} // namespace

DualSenseDevice::DualSenseDevice(QObject* parent)
    : Gamepad(parent)
    , m_disconnectTimer(new QTimer(this))
    , m_reconcileTimer(new QTimer(this))
    , m_topologyTimer(new QTimer(this))
{
    m_clock.start();

    m_disconnectTimer->setSingleShot(true);
    m_disconnectTimer->setInterval(kDisconnectDebounceMs);
    connect(m_disconnectTimer, &QTimer::timeout, this, &DualSenseDevice::finishDisconnect);

    m_reconcileTimer->setSingleShot(true);
    m_reconcileTimer->setInterval(kTopologyDebounceMs);
    connect(m_reconcileTimer, &QTimer::timeout, this, &DualSenseDevice::reconcileDevices);

    m_topologyTimer->setSingleShot(true);
    m_topologyTimer->setInterval(kTopologyDebounceMs);
    connect(m_topologyTimer, &QTimer::timeout, this, &DualSenseDevice::deviceTopologyChanged);
}

DualSenseDevice::~DualSenseDevice()
{
    // Unregister so Windows stops routing WM_INPUT to a dead window.
    registerRawInput(true);

    if (m_hwnd)
        DestroyWindow(static_cast<HWND>(m_hwnd));
}

bool DualSenseDevice::registerRawInput(bool remove)
{
    RAWINPUTDEVICE rid[3]{};
    rid[0].usUsagePage = kUsagePageGeneric;
    rid[0].usUsage     = kUsageGamepad;
    rid[0].dwFlags     = remove ? RIDEV_REMOVE : (RIDEV_INPUTSINK | RIDEV_DEVNOTIFY);
    rid[0].hwndTarget  = remove ? nullptr : static_cast<HWND>(m_hwnd);
    rid[1] = rid[0];
    rid[1].usUsage     = kUsageJoystick;
    rid[2] = rid[0];
    rid[2].usUsage     = kUsageMultiAxis;

    return RegisterRawInputDevices(rid, 3, sizeof(rid[0]));
}

bool DualSenseDevice::start()
{
    WNDCLASSEXW wc{};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = rawInputWndProc;
    wc.hInstance     = GetModuleHandleW(nullptr);
    wc.lpszClassName = kWndClassName;
    RegisterClassExW(&wc);   // ERROR_CLASS_ALREADY_EXISTS is harmless

    HWND hwnd = CreateWindowExW(0, kWndClassName, L"GameHQ Raw Input", 0,
                                0, 0, 0, 0, HWND_MESSAGE, nullptr,
                                wc.hInstance, nullptr);
    if (!hwnd) {
        qWarning() << "Gamepad: message-only window creation failed";
        return false;
    }
    SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    m_hwnd = hwnd;

    // Register for Joystick/Gamepad/MultiAxis usages — pads enumerate as any.
    if (!registerRawInput()) {
        qWarning() << "Gamepad: RegisterRawInputDevices failed, error" << GetLastError();
        return false;
    }
    qInfo() << "Gamepad: Raw Input registered (Sony HID optional - none required to run)";
    // RIDEV_DEVNOTIFY also delivers arrival messages for already-connected
    // devices, but do one synchronous scan so startup logs the initial set.
    reconcileDevices();
    return true;
}

// Query a Raw Input handle and start tracking it if it is a supported
// Sony/DS4-compatible pad. Returns the tracked state, or null for anything
// else (XInput collections, unsupported pads, non-HID handles). Safe to call
// repeatedly with the same handle — arrival bursts and per-report lookups
// hit the "already tracked" fast path.
DualSenseDevice::DeviceState* DualSenseDevice::probeDevice(void* handle)
{
    if (!handle)
        return nullptr;

    auto it = m_devices.find(handle);
    if (it != m_devices.end())
        return &it.value();

    RID_DEVICE_INFO info{};
    info.cbSize = sizeof(info);
    UINT infoSize = sizeof(info);
    if (GetRawInputDeviceInfoW(handle, RIDI_DEVICEINFO, &info, &infoSize)
            == static_cast<UINT>(-1)
        || info.dwType != RIM_TYPEHID)
        return nullptr;

    const QString path = devicePath(handle);
    // Xbox-type pads expose HID collections whose interface path contains
    // "IG_"; they never send usable WM_INPUT reports and are handled by the
    // XInput backend — skip them here so one pad can't drive both backends.
    const bool xinputDevice = path.contains(QLatin1String("IG_"), Qt::CaseInsensitive);
    const int layout = supportedReportLayout(info.hid.dwVendorId, info.hid.dwProductId);

    if (layout == LayoutUnknown || xinputDevice) {
        if (isGamepadUsage(info.hid) && !m_loggedIgnored.contains(path)) {
            m_loggedIgnored.insert(path);
            qInfo() << "Gamepad: ignoring HID device VID"
                    << Qt::hex << info.hid.dwVendorId << "PID" << info.hid.dwProductId
                    << (xinputDevice ? "(XInput device — XInput backend handles it)"
                                     : "(unsupported report layout)");
        }
        return nullptr;
    }

    DeviceState st;
    st.layout    = layout;
    st.vendorId  = info.hid.dwVendorId;
    st.productId = info.hid.dwProductId;
    st.path      = path;
    auto inserted = m_devices.insert(handle, st);
    qInfo() << "Gamepad: tracking" << padName(layout)
            << "VID" << Qt::hex << st.vendorId << "PID" << st.productId
            << Qt::dec << "(" << m_devices.size() << "Sony/DS4 device(s) known)";
    return &inserted.value();
}

void DualSenseDevice::onDeviceChange(bool arrived, void* deviceHandle)
{
    if (arrived)
        probeDevice(deviceHandle);
    else
        removeDevice(deviceHandle);

    // Windows re-enumerates the HID tree in bursts; collapse them into one
    // reconciliation pass + one topology hint for the fallback backends.
    m_reconcileTimer->start();
    m_topologyTimer->start();
}

void DualSenseDevice::removeDevice(void* handle)
{
    auto it = m_devices.find(handle);
    if (it == m_devices.end())
        return;

    const int layout = it->layout;
    const bool wasActive = (handle == m_activeHandle);
    m_devices.erase(it);
    qInfo() << "Gamepad:" << padName(layout) << "removed"
            << "(" << m_devices.size() << "Sony/DS4 device(s) left)";

    if (wasActive) {
        m_activeHandle = nullptr;
        failoverOrScheduleDisconnect();
    }
}

// Full-list sync, debounced behind device-change bursts: tracks any supported
// device we somehow missed and prunes handles Windows no longer lists (a
// removal message can be lost during heavy re-enumeration, e.g. DSX
// recreating its virtual pad).
void DualSenseDevice::reconcileDevices()
{
    QSet<void*> present;

    UINT count = 0;
    if (GetRawInputDeviceList(nullptr, &count, sizeof(RAWINPUTDEVICELIST)) == 0 && count > 0) {
        QByteArray bytes(static_cast<int>(count * sizeof(RAWINPUTDEVICELIST)), Qt::Uninitialized);
        auto* devices = reinterpret_cast<RAWINPUTDEVICELIST*>(bytes.data());
        if (GetRawInputDeviceList(devices, &count, sizeof(RAWINPUTDEVICELIST))
                != static_cast<UINT>(-1)) {
            for (UINT i = 0; i < count; ++i) {
                if (devices[i].dwType != RIM_TYPEHID)
                    continue;
                present.insert(devices[i].hDevice);
                probeDevice(devices[i].hDevice);
            }
        }
    }

    bool lostActive = false;
    for (auto it = m_devices.begin(); it != m_devices.end();) {
        if (present.contains(it.key())) {
            ++it;
            continue;
        }
        qInfo() << "Gamepad:" << padName(it->layout)
                << "pruned (no longer enumerated by Windows)";
        if (it.key() == m_activeHandle) {
            m_activeHandle = nullptr;
            lostActive = true;
        }
        it = m_devices.erase(it);
    }
    if (lostActive)
        failoverOrScheduleDisconnect();

    if (m_devices.isEmpty())
        qInfo() << "Gamepad: no Sony/DS4 Raw Input devices present";
}

// The active pad is gone. If another tracked pad has streamed reports
// recently, promote it right away (its held buttons become the new emitted
// state, releasing anything else). Otherwise arm the disconnect debounce and
// let a late report or reconnect cancel it.
void DualSenseDevice::failoverOrScheduleDisconnect()
{
    const qint64 now = m_clock.elapsed();
    void* best = nullptr;
    qint64 bestAge = 0;
    for (auto it = m_devices.cbegin(); it != m_devices.cend(); ++it) {
        if (!it->reported)
            continue;
        const qint64 age = now - it->lastReportMs;
        if (age <= kActiveSilenceMs * 3 && (!best || age < bestAge)) {
            best = it.key();
            bestAge = age;
        }
    }

    if (best) {
        const DeviceState& st = m_devices[best];
        m_activeHandle = best;
        qInfo() << "Gamepad: failing over to" << padName(st.layout);
        emitEdges(st.buttons);
        return;
    }

    if (m_connectedState && !m_disconnectTimer->isActive())
        m_disconnectTimer->start();
}

void DualSenseDevice::finishDisconnect()
{
    // A reconnect or failover between arm and fire wins over the debounce.
    if (m_activeHandle || !m_connectedState)
        return;

    qInfo() << "Gamepad: Sony/DS4 controller disconnected after debounce";
    emitEdges(0);
    m_connectedState = false;
    emit connected(false);
}

void DualSenseDevice::onRawInput(void* hRawInputV)
{
    HRAWINPUT hRawInput = static_cast<HRAWINPUT>(hRawInputV);

    UINT size = 0;
    if (GetRawInputData(hRawInput, RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER)) != 0
        || size == 0)
        return;

    QByteArray buffer(static_cast<int>(size), Qt::Uninitialized);
    if (GetRawInputData(hRawInput, RID_INPUT, buffer.data(), &size, sizeof(RAWINPUTHEADER)) != size)
        return;

    if (!m_sawInput) {
        m_sawInput = true;
        qInfo() << "Gamepad: first WM_INPUT received (a gamepad is sending reports)";
    }

    auto* ri = reinterpret_cast<RAWINPUT*>(buffer.data());
    if (ri->header.dwType != RIM_TYPEHID)
        return;

    // Look up (or start tracking) this device — reports can beat the arrival
    // message, so an unknown handle is probed here too.
    void* handle = ri->header.hDevice;
    DeviceState* st = probeDevice(handle);
    if (!st)
        return;

    const DWORD count   = ri->data.hid.dwCount;
    const DWORD hidSize = ri->data.hid.dwSizeHid;
    const BYTE* raw     = ri->data.hid.bRawData;
    for (DWORD i = 0; i < count; ++i)
        parseReport(handle, *st, raw + i * hidSize, static_cast<int>(hidSize));
}

void DualSenseDevice::parseReport(void* handle, DeviceState& st,
                                  const unsigned char* d, int len)
{
    if (len < 1)
        return;

    // Locate the button block. USB report 0x01 puts it at byte 8; Bluetooth
    // report 0x31 shifts the whole payload +2 bytes (docs/controller-input.md).
    const unsigned char reportId = d[0];
    int base;
    const bool ds4 = st.layout == LayoutDs4;
    if (reportId == 0x01)
        base = (ds4 || len < 11) ? 5 : 8;
    else if (reportId == 0x11 && ds4)
        base = 7;
    else if (reportId == 0x31 && !ds4)
        base = 10;
    else
        return;

    if (len < base + 3)
        return;

    st.reported = true;
    st.lastReportMs = m_clock.elapsed();

    // Any valid Sony report proves a pad is alive — cancel a pending
    // disconnect. If the active pad was just removed, this report's device
    // becomes the new active pad below.
    if (m_disconnectTimer->isActive()) {
        m_disconnectTimer->stop();
        qInfo() << "Gamepad:" << padName(st.layout)
                << "reports resumed before disconnect debounce";
    }

    const unsigned char b0 = d[base];        // dpad hat (low nibble) + face buttons
    const unsigned char b1 = d[base + 1];    // L1/R1/Share/Options/...
    const unsigned char b2 = d[base + 2];    // PS/touchpad/mute
    const int hat = b0 & 0x0F;

    quint32 s = 0;
    auto set = [&s](int btn) { s |= (1u << btn); };

    if (b0 & 0x10) set(Square);
    if (b0 & 0x20) set(Cross);
    if (b0 & 0x40) set(Circle);
    if (b0 & 0x80) set(Triangle);
    if (b1 & 0x01) set(L1);
    if (b1 & 0x02) set(R1);
    if (b1 & 0x10) set(Share);      // "Create" button
    if (b1 & 0x20) set(Options);
    if (b2 & 0x01) set(PS);

    switch (hat) {                  // 0..7 = 8 directions, 8 = neutral
    case 0: set(DpadUp); break;
    case 1: set(DpadUp);   set(DpadRight); break;
    case 2: set(DpadRight); break;
    case 3: set(DpadDown); set(DpadRight); break;
    case 4: set(DpadDown); break;
    case 5: set(DpadDown); set(DpadLeft); break;
    case 6: set(DpadLeft); break;
    case 7: set(DpadUp);   set(DpadLeft); break;
    default: break;
    }

    // Left stick doubles as the D-pad for menu navigation (overlay request:
    // "D-pad or left stick"). Axes sit 7 bytes before the button block on
    // both encodings (USB base=8 → LX at d[1]; BT base=10 → LX at d[3]),
    // 0..255 with ~128 center. A wide deadzone avoids drift, and HYSTERESIS
    // (kReturnZone < kDeadzone) keeps an axis "active" until the stick
    // returns well inside the center — without this the stick oscillating at
    // the boundary would fire doubled navigation events, and overshooting
    // past center on release would fire a spurious opposite-direction event.
    // emitEdges() below only fires once per direction crossed, same as a
    // real button — no auto-repeat while held, matching the D-pad.
    const int axisBase = base >= 8 ? base - 7 : 1;
    quint32 stick = 0;
    auto stickSet = [&stick](int btn) { stick |= (1u << btn); };
    if (axisBase >= 1 && len > axisBase + 1) {
        constexpr int kCenter = 128;
        constexpr int kDeadzone = 60;     // outer: enter active state
        constexpr int kReturnZone = 30;   // inner: leave active state

        const int lx = d[axisBase];
        const int ly = d[axisBase + 1];

        // X axis — right and left are mutually exclusive, with hysteresis on
        // whichever direction was active last frame.
        const bool wasRight = st.stick & (1u << DpadRight);
        const bool wasLeft  = st.stick & (1u << DpadLeft);
        if (wasRight ? (lx > kCenter + kReturnZone) : (lx > kCenter + kDeadzone))
            stickSet(DpadRight);
        else if (wasLeft ? (lx < kCenter - kReturnZone) : (lx < kCenter - kDeadzone))
            stickSet(DpadLeft);

        // Y axis — down and up are mutually exclusive, same hysteresis.
        const bool wasDown = st.stick & (1u << DpadDown);
        const bool wasUp   = st.stick & (1u << DpadUp);
        if (wasDown ? (ly > kCenter + kReturnZone) : (ly > kCenter + kDeadzone))
            stickSet(DpadDown);
        else if (wasUp ? (ly < kCenter - kReturnZone) : (ly < kCenter - kDeadzone))
            stickSet(DpadUp);
    }
    st.stick = stick;
    s |= stick;

    const bool changed = (s != st.buttons);
    st.buttons = s;

    if (!m_activeHandle) {
        // First reporting pad (or the previous one just vanished) takes over.
        m_activeHandle = handle;
        qInfo() << "Gamepad:" << padName(st.layout) << "active (report id"
                << Qt::hex << reportId << ")";
        if (!m_connectedState) {
            m_connectedState = true;
            emit connected(true);
        }
        emitEdges(s);
        return;
    }

    if (handle == m_activeHandle) {
        // On any button change, dump the raw report so offsets (esp. the BT
        // +2 shift) can be confirmed/corrected against real hardware.
        if (changed) {
            const int n = qMin(len, 16);
            QString hex;
            for (int i = 0; i < n; ++i)
                hex += QString::asprintf("%02X ", d[i]);
            qInfo().noquote() << "Gamepad: report" << hex.trimmed();
        }
        emitEdges(s);
        return;
    }

    // Report from a non-active pad: keep its state current, but only steal
    // the active role when it shows a real input change while the active pad
    // has gone silent (DSX swapped its virtual pad, HidHide hid the physical
    // one mid-session, ...). This is what stops two mirrored pads from
    // double-firing every press.
    auto activeIt = m_devices.constFind(m_activeHandle);
    const bool activeSilent = activeIt == m_devices.cend()
        || (st.lastReportMs - activeIt->lastReportMs) > kActiveSilenceMs;
    if (changed && activeSilent) {
        qInfo() << "Gamepad: switching active pad to" << padName(st.layout)
                << "(previous pad went silent)";
        m_activeHandle = handle;
        if (!m_connectedState) {
            m_connectedState = true;
            emit connected(true);
        }
        emitEdges(s);
    }
}

void DualSenseDevice::emitEdges(quint32 buttons)
{
    const quint32 changed = buttons ^ m_emittedButtons;
    if (changed == 0)
        return;
    for (int b = 0; b < MaxButtons; ++b) {
        const quint32 mask = 1u << b;
        if (!(changed & mask))
            continue;
        if (buttons & mask)
            publishButtonPressed(b);
        else
            publishButtonReleased(b);
    }
    m_emittedButtons = buttons;
}

ControlId::DeviceProfile DualSenseDevice::profile() const
{
    if (!m_activeHandle || !m_devices.contains(m_activeHandle))
        return {};
    const DeviceState& st = m_devices.value(m_activeHandle);
    return ControlId::DeviceProfile{
        QStringLiteral("Sony Raw Input"),
        QStringLiteral("%1:%2").arg(st.vendorId, 4, 16, QLatin1Char('0'))
                                .arg(st.productId, 4, 16, QLatin1Char('0')),
        ControlId::ControllerFamily::PlayStation,
        QString::fromLatin1(padName(st.layout)),
    };
}
