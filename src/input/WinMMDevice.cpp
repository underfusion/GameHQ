#include "input/WinMMDevice.h"

#include "input/StickNav.h"

#include <QDebug>
#include <QTimer>

#include <windows.h>
#include <mmsystem.h>

namespace {
constexpr int kPollMs = 50;
constexpr int kRescanMs = 2000;
constexpr UINT kMaxSlots = 16;

quint32 mapWinMMState(const JOYINFOEX& info, bool ds4Layout)
{
    quint32 s = 0;
    auto set = [&s](int btn) { s |= (1u << btn); };
    const quint32 b = info.dwButtons;

    if (ds4Layout) {
        // Sony DS4/DualSense DirectInput button order (also used by DSX's
        // virtual Sony pads):
        //   0=Square 1=Cross 2=Circle 3=Triangle 4=L1 5=R1 6=L2 7=R2
        //   8=Share 9=Options 10=L3 11=R3 12=PS 13=Touchpad
        // This is what makes Share-screenshot and PS-overlay work when the
        // pad is only reachable through WinMM.
        if (b & 0x0001) set(Gamepad::Square);
        if (b & 0x0002) set(Gamepad::Cross);
        if (b & 0x0004) set(Gamepad::Circle);
        if (b & 0x0008) set(Gamepad::Triangle);
        if (b & 0x0010) set(Gamepad::L1);
        if (b & 0x0020) set(Gamepad::R1);
        if (b & 0x0100) set(Gamepad::Share);
        if (b & 0x0200) set(Gamepad::Options);
        if (b & 0x1000) set(Gamepad::PS);
        // Previously discarded — now exposed as generic (unnamed-position)
        // buttons so they stay bindable: 6=L2 7=R2 10=L3 11=R3 13=Touchpad.
        if (b & 0x0040) set(Gamepad::GenericButtonBase + 0);
        if (b & 0x0080) set(Gamepad::GenericButtonBase + 1);
        if (b & 0x0400) set(Gamepad::GenericButtonBase + 2);
        if (b & 0x0800) set(Gamepad::GenericButtonBase + 3);
        if (b & 0x2000) set(Gamepad::GenericButtonBase + 4);
    } else {
        // Xbox-style joystick button numbering:
        //   0=A, 1=B, 2=X, 3=Y, 4=LB, 5=RB, 6=Back, 7=Start, 8=LThumb, 9=RThumb
        // Covers DSX virtual Xbox mode and generic DirectInput pads.
        if (b & 0x01) set(Gamepad::Cross);       // A
        if (b & 0x02) set(Gamepad::Circle);      // B
        if (b & 0x04) set(Gamepad::Square);      // X
        if (b & 0x08) set(Gamepad::Triangle);    // Y
        if (b & 0x10) set(Gamepad::L1);          // LB
        if (b & 0x20) set(Gamepad::R1);          // RB
        if (b & 0x40) set(Gamepad::Share);       // Back
        if (b & 0x80) set(Gamepad::Options);     // Start
        // Previously discarded — now exposed as generic buttons.
        if (b & 0x100) set(Gamepad::GenericButtonBase + 0);   // LThumb
        if (b & 0x200) set(Gamepad::GenericButtonBase + 1);   // RThumb
    }

    // Some drivers report "centered" as 0xFFFFFFFF instead of the 16-bit
    // JOY_POVCENTERED (0xFFFF) — treat anything outside 0..35999 as neutral.
    if (info.dwPOV < 36000) {
        if (info.dwPOV >= 31500 || info.dwPOV <= 4500) set(Gamepad::DpadUp);
        if (info.dwPOV >= 4500 && info.dwPOV <= 13500) set(Gamepad::DpadRight);
        if (info.dwPOV >= 13500 && info.dwPOV <= 22500) set(Gamepad::DpadDown);
        if (info.dwPOV >= 22500 && info.dwPOV <= 31500) set(Gamepad::DpadLeft);
    }

    // Unsigned 0..65535 axes centered mid-range, Y growing downward. No
    // hysteresis here (return zone == deadzone), matching how this backend has
    // always behaved.
    constexpr StickNav::AxisConfig kNav{ 32767, 16000, 16000, false };
    s |= StickNav::bits(kNav, static_cast<int>(info.dwXpos), static_cast<int>(info.dwYpos));

    return s;
}
} // namespace

WinMMDevice::WinMMDevice(QObject* parent)
    : Gamepad(parent)
    , m_pollTimer(new QTimer(this))
    , m_rescanTimer(new QTimer(this))
{
    m_pollTimer->setTimerType(Qt::PreciseTimer);
    m_pollTimer->setInterval(kPollMs);
    connect(m_pollTimer, &QTimer::timeout, this, &WinMMDevice::poll);

    m_rescanTimer->setInterval(kRescanMs);
    connect(m_rescanTimer, &QTimer::timeout, this, &WinMMDevice::rescan);
}

WinMMDevice::~WinMMDevice() = default;

bool WinMMDevice::start()
{
    rescan();
    if (!m_connected) {
        qInfo() << "Gamepad: WinMM no joystick present — waiting for device changes";
        m_rescanTimer->start();
    }
    return true;
}

ControlId::DeviceProfile WinMMDevice::profile() const
{
    if (!m_connected)
        return {};
    return ControlId::DeviceProfile{
        QStringLiteral("WinMM joystick"),
        QStringLiteral("%1:%2").arg(m_vendorId, 4, 16, QLatin1Char('0'))
                                .arg(m_productId, 4, 16, QLatin1Char('0')),
        m_ds4Layout ? ControlId::ControllerFamily::PlayStation : ControlId::ControllerFamily::Xbox,
        m_ds4Layout ? QStringLiteral("WinMM joystick (Sony layout)")
                    : QStringLiteral("WinMM joystick (Xbox layout)"),
    };
}

void WinMMDevice::rescan()
{
    if (m_connected)
        return;

    const UINT numDevs = joyGetNumDevs();
    for (UINT id = 0; id < numDevs && id < kMaxSlots; ++id) {
        JOYINFOEX info{};
        info.dwSize = sizeof(info);
        info.dwFlags = JOY_RETURNALL;

        if (joyGetPosEx(id, &info) != JOYERR_NOERROR)
            continue;

        m_activeId = id;
        m_connected = true;
        m_prevButtons = 0;

        // The button ORDER depends on the pad family: Sony pads (and DSX's
        // virtual Sony pads) put Share at button 8 and PS at button 12,
        // Xbox-style pads put Back/Start at 6/7. joyGetDevCaps exposes the
        // vendor/product id, so pick the mapping from that.
        JOYCAPSW caps{};
        quint32 mid = 0, pid = 0;
        if (joyGetDevCapsW(id, &caps, sizeof(caps)) == JOYERR_NOERROR) {
            mid = caps.wMid;
            pid = caps.wPid;
        }
        m_ds4Layout = (mid == 0x054C)
            || (mid == 0x11FF && pid == 0x0847)
            || (mid == 0x3670 && pid == 0x0902);
        m_vendorId = mid;
        m_productId = pid;
        qInfo() << "Gamepad: WinMM joystick connected (JOYSTICKID" << (id + 1)
                << ") VID" << Qt::hex << mid << "PID" << pid << Qt::dec
                << (m_ds4Layout ? "— Sony button layout" : "— Xbox button layout");
        m_rescanTimer->stop();
        m_pollTimer->start();
        emit connected(true);
        return;
    }
}

void WinMMDevice::poll()
{
    if (!m_connected)
        return;

    JOYINFOEX info{};
    info.dwSize = sizeof(info);
    info.dwFlags = JOY_RETURNALL;

    const MMRESULT result = joyGetPosEx(m_activeId, &info);

    if (result == JOYERR_UNPLUGGED) {
        disconnectActive();
        return;
    }

    if (result != JOYERR_NOERROR)
        return;   // transient error — keep the slot, skip this tick

    emitEdges(mapWinMMState(info, m_ds4Layout));
}

void WinMMDevice::disconnectActive()
{
    qInfo() << "Gamepad: WinMM joystick disconnected (JOYSTICKID" << (m_activeId + 1) << ")";
    // Release anything still held before announcing the disconnect, so no
    // button can stay logically stuck across an unplug.
    emitEdges(0);
    m_connected = false;
    m_activeId = UINT_MAX;
    m_pollTimer->stop();
    m_rescanTimer->start();
    emit connected(false);
}

void WinMMDevice::emitEdges(quint32 buttons)
{
    const quint32 changed = buttons ^ m_prevButtons;
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
    m_prevButtons = buttons;
}
