#pragma once
#include <QString>

// Canonical, device-neutral control identity.
// Bindings persist these codes, never a display label — the same physical
// button keeps the same code across a controller swap, while the label shown
// to the user changes with the selected family. Positions are physical, not
// cultural: PlayStation Cross and Nintendo B are both "gamepad.face_south".
namespace ControlId {

inline const QString FaceSouth      = QStringLiteral("gamepad.face_south");
inline const QString FaceEast       = QStringLiteral("gamepad.face_east");
inline const QString FaceNorth      = QStringLiteral("gamepad.face_north");
inline const QString FaceWest       = QStringLiteral("gamepad.face_west");
inline const QString ShoulderLeft   = QStringLiteral("gamepad.shoulder_left");
inline const QString ShoulderRight  = QStringLiteral("gamepad.shoulder_right");
// Triggers are treated as buttons, not axes: every action bound to them here is
// a discrete step (zoom in/out), so a pressed/released edge is all we need.
inline const QString TriggerLeft    = QStringLiteral("gamepad.trigger_left");
inline const QString TriggerRight   = QStringLiteral("gamepad.trigger_right");
inline const QString DpadUp         = QStringLiteral("gamepad.dpad_up");
inline const QString DpadDown       = QStringLiteral("gamepad.dpad_down");
inline const QString DpadLeft       = QStringLiteral("gamepad.dpad_left");
inline const QString DpadRight      = QStringLiteral("gamepad.dpad_right");
inline const QString Menu           = QStringLiteral("gamepad.menu");    // Options / Menu / +
inline const QString Guide          = QStringLiteral("gamepad.guide");   // PS / Xbox Guide / Home
inline const QString Capture        = QStringLiteral("gamepad.capture"); // Share / View / Capture

// Unknown WinMM buttons stay bindable without a known position: "gamepad.button.3".
QString genericButton(int index);

// True for any gamepad.button.N code produced by genericButton().
bool isGenericButton(const QString& code);

enum class ControllerFamily {
    PlayStation,
    Xbox,
    Nintendo,
    Generic
};

// Per-device identity carried alongside the canonical code so the binding
// editor can show "DualSense" instead of just "controller".
struct DeviceProfile {
    QString backend;          // "Sony Raw Input", "XInput", "WinMM joystick"
    QString fingerprint;      // stable per-device id, e.g. "054C:0CE6" (VID:PID)
    ControllerFamily family = ControllerFamily::Generic;
    QString displayName;      // "DualSense", "Xbox Wireless Controller", ...
};

// Display label for a canonical code under a given family. Unknown/generic
// codes fall back to a plain description (e.g. "Button 3").
QString label(const QString& code, ControllerFamily family);

} // namespace ControlId
