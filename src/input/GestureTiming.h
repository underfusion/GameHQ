#pragma once

#include "input/InputPatternRecognizer.h"

#include <optional>

// The three numbers every gesture is measured against, and the rules that keep
// them sane.
//
// They live here rather than inline in InputEngine because two very different
// places need the same answer: the recognizer, which waits them out, and the
// binding editor, which tells the user how long it will wait. A notice reading
// "waits up to 300 ms" has to be the same 300 the runtime actually uses.
//
// Every bound is a real constraint, not a defensive guess:
//  - hold below 250 ms is indistinguishable from a tap; above 10 s nobody waits;
//  - a multi-tap interval below ~120 ms cannot be hit twice by a human hand,
//    and above 800 ms a single tap feels broken;
//  - the chord window is the latency a shared first button pays, so it is
//    capped harder than the hold: a full second of "did it work?" is not worth
//    any combination.
namespace GestureTiming {

inline constexpr int kMinHoldMs = 250;
inline constexpr int kMaxHoldMs = 10000;
inline constexpr int kMinMultiTapMs = 120;
inline constexpr int kMaxMultiTapMs = 800;
inline constexpr int kMinChordWindowMs = 120;
inline constexpr int kMaxChordWindowMs = 1000;

int clampHoldMs(int milliseconds);
int clampMultiTapIntervalMs(int milliseconds);
int clampChordWindowMs(int milliseconds);

// One-release migration of the pre-0.7.3 `input.share_hold_ms` key.
//
// Override-only on purpose: a user who never changed the hold threshold has no
// override to carry, and writing one would freeze today's default into their
// config forever — a later change to the shipped default would then silently
// not apply to them. Returns the value `input.default_hold_ms` should be given,
// or nothing when there is nothing to migrate.
std::optional<int> migratedHoldMs(bool hasNewOverride, bool hasLegacyOverride,
                                  int legacyValue);

// Builds the recognizer's timing from raw (already-read) config values.
InputPatternRecognizer::Timing fromValues(int holdMs, int multiTapMs, int chordWindowMs);

// One line for the diagnostics export and the startup log.
QString describe(const InputPatternRecognizer::Timing& timing);

} // namespace GestureTiming
