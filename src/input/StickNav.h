#pragma once

#include "input/Gamepad.h"

// The "left stick doubles as the D-pad for menu navigation" rule, shared by
// every backend (docs/controller-input.md). Each backend used to hand-roll it
// against its own raw axis format, which made three traps easy to hit and hard
// to see:
//
//   1. AXIS POLARITY IS NOT UNIVERSAL. DualSense and WinMM report Y with
//      positive = DOWN; XInput reports it signed with positive = UP. Getting
//      this wrong inverts menu navigation and looks like a binding bug.
//   2. THE TWO DIRECTIONS ON AN AXIS MUST BE MUTUALLY EXCLUSIVE, so the
//      caller never sees Left+Right in the same frame.
//   3. HYSTERESIS IS OPT-IN. With returnZone < deadzone an axis stays
//      "active" until the stick returns well inside center; without it, a
//      stick oscillating at the boundary fires doubled navigation events and
//      overshoot past center on release fires a spurious opposite direction.
//
// Only the STRUCTURE is shared. Deadzone values stay per-backend on purpose:
// they are tuned against each pad's raw range and its own feel, and are not
// interchangeable (see AxisConfig users). Backends that historically ran with
// no hysteresis keep none — set returnZone == deadzone and the rule collapses
// to a plain threshold, which is exactly what they did before.
namespace StickNav {

struct AxisConfig {
    int center;          // raw value with the stick at rest
    int deadzone;        // outer: distance from center that ENTERS a direction
    int returnZone;      // inner: distance that LEAVES it; == deadzone = no hysteresis
    bool yPositiveIsUp;  // false when the raw Y axis grows downward
};

// True once |v - center| clears the threshold on the requested side. The
// threshold tightens to returnZone while the direction is already held, which
// is what makes the state sticky.
inline bool axisActive(int v, const AxisConfig& cfg, bool held, bool positiveSide)
{
    const int threshold = held ? cfg.returnZone : cfg.deadzone;
    return positiveSide ? (v > cfg.center + threshold)
                        : (v < cfg.center - threshold);
}

// Map one stick's raw x/y onto Gamepad::Dpad* bits. `prev` is the bitmask this
// function returned last frame; it only matters when hysteresis is enabled, so
// backends without it can pass 0.
inline quint32 bits(const AxisConfig& cfg, int x, int y, quint32 prev = 0)
{
    quint32 out = 0;
    auto set = [&out](int btn) { out |= (1u << btn); };
    auto held = [prev](int btn) { return (prev & (1u << btn)) != 0; };

    if (axisActive(x, cfg, held(Gamepad::DpadRight), true))
        set(Gamepad::DpadRight);
    else if (axisActive(x, cfg, held(Gamepad::DpadLeft), false))
        set(Gamepad::DpadLeft);

    const int growing = cfg.yPositiveIsUp ? Gamepad::DpadUp : Gamepad::DpadDown;
    const int shrinking = cfg.yPositiveIsUp ? Gamepad::DpadDown : Gamepad::DpadUp;
    if (axisActive(y, cfg, held(growing), true))
        set(growing);
    else if (axisActive(y, cfg, held(shrinking), false))
        set(shrinking);

    return out;
}

} // namespace StickNav
