#pragma once

#include <QtGlobal>

namespace ControllerArbitration
{
// Mirrored backends normally report the same physical press within a few
// milliseconds of each other (a remapper like DSX feeds one pad into Raw
// Input, XInput and WinMM at once). Any cross-backend event this close to the
// active backend's last control is treated as a suspect mirror of that press.
inline constexpr qint64 BackendDuplicateWindowMs = 100;

// Deduplication and takeover are different questions with different clocks.
// Only once the active backend has produced nothing for this long has it
// genuinely gone silent; activity on another backend may then claim the
// active role. Between the two thresholds an event is real but the active
// backend is alive, so it is neither mirrored nor a failover — it is dropped.
inline constexpr qint64 BackendTakeoverSilenceMs = 1000;

// How long another backend must keep producing input, entirely alone, before
// it is promoted without waiting out BackendTakeoverSilenceMs. Waiting for the
// full silence threshold is correct but expensive: toggling DSX between Sony
// and Xbox mode could cost the user up to a second of dropped presses. A
// candidate that is still producing input a quarter of a second after its
// first event — while the active backend has said nothing in between — is a
// real failover, not a mirror, because a mirroring remapper feeds both paths
// at once and would have kept the active backend alive.
inline constexpr qint64 BackendCandidateConfirmMs = 250;

// The takeover contract (the state machine over the active-backend role):
//
//  - The active backend keeps the role while it is connected and has produced
//    real input within BackendTakeoverSilenceMs. Disconnect failover is not
//    decided here — InputEngine::updateActiveBackend() re-picks by priority
//    the moment the active backend drops.
//  - An event inside BackendDuplicateWindowMs never takes over, whether its
//    canonical control id matches the active backend's last control or not:
//    remappers translate ids between APIs, so a *different* id close in time
//    is no proof of a different pad. (Letting different ids through was the
//    bug that double-acted one physical press.)
//  - A higher-priority backend that demonstrably sees the SAME physical
//    device (equal fingerprint) upgrades immediately once outside the mirror
//    window — waiting out the silence threshold would only prolong the worse
//    path.
//  - Anything else waits for real silence — or for candidateMayConfirm()
//    below, which shortens that wait when the candidate proves itself.
inline bool backendMayTakeOver(bool activeHasReportedControl,
                               qint64 activeLastControlMs,
                               qint64 candidateControlMs,
                               bool candidateHigherPrioritySameDevice = false)
{
    if (!activeHasReportedControl)
        return true;
    const qint64 sinceActive = candidateControlMs - activeLastControlMs;
    if (sinceActive <= BackendDuplicateWindowMs)
        return false;
    if (candidateHigherPrioritySameDevice)
        return true;
    return sinceActive > BackendTakeoverSilenceMs;
}

// The pending-candidate rule: a backend that backendMayTakeOver() refused may
// still be promoted once it has demonstrably carried input on its own.
//
// `candidateFirstMs` is the candidate's first event of the current pending
// run; the caller restarts that run whenever the active backend reports
// anything, so reaching this function at all means the active backend has been
// silent for the whole run. Requirements, in order:
//
//  - the active backend must not have spoken since the run began (mirrored
//    traffic keeps it alive and must never be promoted this way);
//  - the newest candidate event must be outside the mirror window, so a press
//    the active backend just acted on cannot be re-dispatched;
//  - the run must have lasted at least BackendCandidateConfirmMs, so a single
//    stray event still cannot move the role.
inline bool candidateMayConfirm(qint64 candidateFirstMs,
                                qint64 candidateNowMs,
                                qint64 activeLastControlMs)
{
    if (activeLastControlMs >= candidateFirstMs)
        return false;
    if (candidateNowMs - activeLastControlMs <= BackendDuplicateWindowMs)
        return false;
    return candidateNowMs - candidateFirstMs >= BackendCandidateConfirmMs;
}

// The held-press rule. XInput and WinMM report state *changes*, so a user who
// presses a button once and lets go produces exactly one event on the new
// backend — a confirmation scheme that waits for a second event would lose
// that press for good. The first press of a candidate run is therefore held,
// not dropped, and this decides its fate once the confirmation window closes:
// deliver it only if the active backend never spoke after it was pressed
// (anything else means the candidate was mirroring the active backend), and
// only once the window has actually elapsed.
inline bool heldPressSurvives(qint64 pressedMs, qint64 activeLastControlMs,
                              qint64 resolveMs)
{
    if (activeLastControlMs >= pressedMs)
        return false;
    // A mirror trails the original: the remapper (or the WinMM compatibility
    // view of an XInput pad) re-reports the press a few milliseconds AFTER
    // the active backend already delivered it, so the active backend spoke
    // before the candidate press — never after — and the check above cannot
    // catch it. A press born inside the mirror window of the active backend's
    // last control is that trailing mirror and must die here too; replaying
    // it was the 0.7.3 "one tap moves two rows" bug. Compared on the pressed
    // side so the caller's "active never spoke" sentinel (qint64 min) cannot
    // overflow a subtraction.
    if (activeLastControlMs >= pressedMs - BackendDuplicateWindowMs)
        return false;
    return resolveMs - pressedMs >= BackendCandidateConfirmMs;
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
