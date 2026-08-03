#pragma once

#include <QString>
#include <QStringList>

// Decides what identity an XInput slot's bindings should be keyed on.
//
// XInput itself only knows slot numbers, so historically device-specific
// overrides were fingerprinted "xinput.slotN" — which means two different
// pads that ever occupy the same slot silently inherit each other's custom
// bindings. Raw Input, however, enumerates the same physical devices as HID
// collections whose path carries "IG_", complete with VID/PID. When exactly
// one XInput-class device is present and exactly one slot is connected, the
// correlation is unambiguous and the slot can be keyed on the real hardware
// identity ("vvvv:pppp"). Any ambiguity — no IG_ device visible (HidHide),
// several distinct devices, several connected slots — falls back to the
// honest legacy slot fingerprint rather than guessing.
namespace ControllerIdentity
{
QString legacySlotFingerprint(int slot);
bool isLegacySlotFingerprint(const QString& fingerprint);

// `xinputClassIdentities`: distinct "vvvv:pppp" identities of IG_ devices
// Raw Input currently sees. `connectedSlotCount`: XInput slots in use.
QString resolveXInputFingerprint(const QStringList& xinputClassIdentities,
                                 int connectedSlotCount, int slot);
} // namespace ControllerIdentity
