#include "input/GestureTiming.h"

#include <QString>
#include <QtGlobal>

namespace GestureTiming {

int clampHoldMs(int milliseconds)
{
    return qBound(kMinHoldMs, milliseconds, kMaxHoldMs);
}

int clampMultiTapIntervalMs(int milliseconds)
{
    return qBound(kMinMultiTapMs, milliseconds, kMaxMultiTapMs);
}

int clampChordWindowMs(int milliseconds)
{
    return qBound(kMinChordWindowMs, milliseconds, kMaxChordWindowMs);
}

std::optional<int> migratedHoldMs(bool hasNewOverride, bool hasLegacyOverride, int legacyValue)
{
    // An explicit new value always wins: the user has already answered this
    // question under the new name.
    if (hasNewOverride || !hasLegacyOverride)
        return std::nullopt;
    return clampHoldMs(legacyValue);
}

InputPatternRecognizer::Timing fromValues(int holdMs, int multiTapMs, int chordWindowMs)
{
    InputPatternRecognizer::Timing timing;
    timing.defaultHoldMs = clampHoldMs(holdMs);
    timing.multiTapIntervalMs = clampMultiTapIntervalMs(multiTapMs);
    timing.chordWindowMs = clampChordWindowMs(chordWindowMs);
    return timing;
}

QString describe(const InputPatternRecognizer::Timing& timing)
{
    return QStringLiteral("hold %1 ms, multi-tap %2 ms, chord window %3 ms")
        .arg(timing.defaultHoldMs)
        .arg(timing.multiTapIntervalMs)
        .arg(timing.chordWindowMs);
}

} // namespace GestureTiming
