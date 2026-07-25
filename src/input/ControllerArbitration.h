#pragma once

#include <QtGlobal>

namespace ControllerArbitration
{
// Mirrored backends normally report the same press within a few milliseconds.
// Outside this window, activity from another backend is treated as a genuine
// failover signal instead of being discarded behind a stale "connected" flag.
inline constexpr qint64 BackendDuplicateWindowMs = 100;

inline bool backendMayTakeOver(bool activeHasReportedControl,
                               bool controlsMatch,
                               qint64 activeLastControlMs,
                               qint64 candidateControlMs)
{
    return !activeHasReportedControl
        || !controlsMatch
        || candidateControlMs - activeLastControlMs > BackendDuplicateWindowMs;
}

// Within the Sony Raw Input backend, prefer a higher-quality device
// immediately (physical Sony > virtual DualSense > virtual DS4). A lower or
// equal-priority device may still rescue input after the active device has
// stopped producing button/stick changes.
inline bool sonyDeviceMayTakeOver(bool candidateChanged,
                                  int candidatePriority,
                                  int activePriority,
                                  qint64 candidateChangeMs,
                                  qint64 activeLastChangeMs,
                                  qint64 idleThresholdMs)
{
    if (!candidateChanged)
        return false;
    return candidatePriority > activePriority
        || candidateChangeMs - activeLastChangeMs > idleThresholdMs;
}
} // namespace ControllerArbitration
