#pragma once

#include "input/ControlId.h"

#include <QString>
#include <QStringList>

namespace ModernInput {

// Complete map of every GameInputGamepadButtons flag the pinned v3 header
// (third_party/gameinput/include/GameInput.h) exposes onto the canonical
// control ids bindings persist. Flags are mirrored numerically so this header
// stays buildable without the vendored SDK header; tst_providerintegration
// asserts the values against the real header's enum.
//
// Left-thumbstick DIRECTION flags map onto the d-pad navigation ids — the
// same merge every legacy backend performs (see DualSenseDevice's
// decodeStickNav). Right-thumbstick direction flags have no canonical
// control id in this app and are deliberately unmapped rather than invented;
// they return an empty id and must not be routed.
namespace StandardControlMap {

constexpr quint32 Menu                 = 0x00000001u;
constexpr quint32 View                 = 0x00000002u;
constexpr quint32 FaceA                = 0x00000004u;
constexpr quint32 FaceB                = 0x00000008u;
constexpr quint32 FaceX                = 0x00000010u;
constexpr quint32 FaceY                = 0x00000020u;
constexpr quint32 DpadUp               = 0x00000040u;
constexpr quint32 DpadDown             = 0x00000080u;
constexpr quint32 DpadLeft             = 0x00000100u;
constexpr quint32 DpadRight            = 0x00000200u;
constexpr quint32 LeftShoulder         = 0x00000400u;
constexpr quint32 RightShoulder        = 0x00000800u;
constexpr quint32 LeftThumbstick       = 0x00001000u;
constexpr quint32 RightThumbstick      = 0x00002000u;
constexpr quint32 FaceC                = 0x00004000u;
constexpr quint32 FaceZ                = 0x00008000u;
constexpr quint32 LeftTriggerButton    = 0x00010000u;
constexpr quint32 RightTriggerButton   = 0x00020000u;
constexpr quint32 LeftThumbstickUp     = 0x00040000u;
constexpr quint32 LeftThumbstickDown   = 0x00080000u;
constexpr quint32 LeftThumbstickLeft   = 0x00100000u;
constexpr quint32 LeftThumbstickRight  = 0x00200000u;
constexpr quint32 RightThumbstickUp    = 0x00400000u;
constexpr quint32 RightThumbstickDown  = 0x00800000u;
constexpr quint32 RightThumbstickLeft  = 0x01000000u;
constexpr quint32 RightThumbstickRight = 0x02000000u;
constexpr quint32 PaddleLeft1          = 0x04000000u;
constexpr quint32 PaddleLeft2          = 0x08000000u;
constexpr quint32 PaddleRight1        = 0x10000000u;
constexpr quint32 PaddleRight2        = 0x20000000u;

// Every flag with a canonical meaning. An unmapped flag returns empty.
inline QString controlFor(quint32 flag)
{
    switch (flag) {
    case Menu:                return ControlId::Menu;
    case View:                return ControlId::ViewBack;
    case FaceA:               return ControlId::FaceSouth;
    case FaceB:               return ControlId::FaceEast;
    case FaceX:               return ControlId::FaceWest;
    case FaceY:               return ControlId::FaceNorth;
    case FaceC:               return ControlId::FaceC;
    case FaceZ:               return ControlId::FaceZ;
    case DpadUp:              return ControlId::DpadUp;
    case DpadDown:            return ControlId::DpadDown;
    case DpadLeft:            return ControlId::DpadLeft;
    case DpadRight:           return ControlId::DpadRight;
    case LeftShoulder:        return ControlId::ShoulderLeft;
    case RightShoulder:       return ControlId::ShoulderRight;
    case LeftTriggerButton:   return ControlId::TriggerLeft;
    case RightTriggerButton:  return ControlId::TriggerRight;
    case LeftThumbstick:      return ControlId::ThumbLeft;
    case RightThumbstick:     return ControlId::ThumbRight;
    case LeftThumbstickUp:    return ControlId::DpadUp;
    case LeftThumbstickDown:  return ControlId::DpadDown;
    case LeftThumbstickLeft:  return ControlId::DpadLeft;
    case LeftThumbstickRight: return ControlId::DpadRight;
    case PaddleLeft1:         return ControlId::PaddleLeft1;
    case PaddleLeft2:         return ControlId::PaddleLeft2;
    case PaddleRight1:        return ControlId::PaddleRight1;
    case PaddleRight2:        return ControlId::PaddleRight2;
    default:                  return {};
    }
}

// Canonical ids for every set bit in `mask`, unmapped bits skipped.
inline QStringList controlsFor(quint32 mask)
{
    QStringList controls;
    for (quint32 bit = 1; bit != 0; bit <<= 1) {
        if ((mask & bit) == 0)
            continue;
        const QString control = controlFor(bit);
        if (!control.isEmpty())
            controls.push_back(control);
    }
    return controls;
}

} // namespace StandardControlMap
} // namespace ModernInput
