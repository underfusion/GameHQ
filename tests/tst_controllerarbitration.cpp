#include <QtTest>

#include "input/ControllerArbitration.h"

#include <limits>

// The takeover contract: the active backend keeps the role while it is alive;
// close-in-time events from another backend are suspect mirrors regardless of
// control id; only real silence — or a higher-priority path to the same
// physical device — moves the role. Disconnect failover is InputEngine's
// updateActiveBackend() and is deliberately not decided here.
class ControllerArbitrationTest : public QObject
{
    Q_OBJECT

private slots:
    void backendWithoutActivityYieldsImmediately()
    {
        QVERIFY(ControllerArbitration::backendMayTakeOver(false, 0, 1));
    }

    void mirroredPressInsideWindowIsSuppressed()
    {
        // Same physical press surfacing on a second API a few ms later.
        QVERIFY(!ControllerArbitration::backendMayTakeOver(true, 1000, 1005));
        QVERIFY(!ControllerArbitration::backendMayTakeOver(
            true, 1000, 1000 + ControllerArbitration::BackendDuplicateWindowMs));
    }

    void differentControlIdInsideWindowIsStillSuppressed()
    {
        // The old rule let a mirror through when the remapper translated the
        // control id between APIs — one press became two actions. Id equality
        // is no longer part of the decision, so this cannot regress silently.
        QVERIFY(!ControllerArbitration::backendMayTakeOver(true, 1000, 1005));
        QVERIFY(!ControllerArbitration::backendMayTakeOver(true, 1000, 1099));
    }

    void realEventWhileActiveIsAliveIsDroppedNotPromoted()
    {
        // Outside the mirror window but inside the silence threshold: the
        // event is genuine, but the active backend is still producing input.
        // No takeover — and no bounce.
        QVERIFY(!ControllerArbitration::backendMayTakeOver(true, 1000, 1101));
        QVERIFY(!ControllerArbitration::backendMayTakeOver(
            true, 1000, 1000 + ControllerArbitration::BackendTakeoverSilenceMs));
    }

    void genuineSilenceAllowsFailover()
    {
        QVERIFY(ControllerArbitration::backendMayTakeOver(
            true, 1000, 1001 + ControllerArbitration::BackendTakeoverSilenceMs));
    }

    void samePhysicalDeviceUpgradesAfterTheMirrorWindow()
    {
        // Sony Raw Input seeing the pad WinMM is currently driving: upgrade
        // without waiting out the silence threshold...
        QVERIFY(ControllerArbitration::backendMayTakeOver(true, 1000, 1101, true));
        // ...but never inside the mirror window, where the "upgrade" would
        // re-dispatch the same physical press the active backend just acted on.
        QVERIFY(!ControllerArbitration::backendMayTakeOver(true, 1000, 1005, true));
    }

    // The pending-candidate rule exists so a real backend switch (DSX toggled
    // between Sony and Xbox mode) does not cost the user a full second of
    // dropped presses while the silence threshold runs down.
    void sustainedCandidateConfirmsBeforeTheSilenceThreshold()
    {
        // Active backend last spoke at 1000; the candidate's run starts at
        // 1101 (outside the mirror window) and is still going at 1351.
        QVERIFY(ControllerArbitration::candidateMayConfirm(1101, 1351, 1000));
        // Promotion lands well before the silence threshold would allow it.
        QVERIFY(!ControllerArbitration::backendMayTakeOver(true, 1000, 1351));
    }

    void singleCandidateEventNeverConfirms()
    {
        // One event cannot move the role: the run has no duration yet, and a
        // short run stays below the confirmation window.
        QVERIFY(!ControllerArbitration::candidateMayConfirm(1101, 1101, 1000));
        QVERIFY(!ControllerArbitration::candidateMayConfirm(
            1101, 1100 + ControllerArbitration::BackendCandidateConfirmMs, 1000));
    }

    void activeBackendSpeakingDuringTheRunBlocksConfirmation()
    {
        // Mirrored traffic keeps the active backend alive, so its last control
        // lands at or after the candidate's first event — the classic DSX
        // one-pad-two-APIs case, which must never confirm.
        QVERIFY(!ControllerArbitration::candidateMayConfirm(1101, 2000, 1101));
        QVERIFY(!ControllerArbitration::candidateMayConfirm(1101, 2000, 1500));
    }

    void confirmationNeverFiresInsideTheMirrorWindow()
    {
        // A long-running candidate whose newest event is a few ms behind the
        // active backend's would re-dispatch the press just acted on.
        QVERIFY(!ControllerArbitration::candidateMayConfirm(1000, 2005, 2000));
    }

    // The case that motivated holding the press instead of dropping it: XInput
    // and WinMM report state changes, so one tap on a genuinely new backend is
    // ONE event. Waiting for a second one would lose it for good.
    void oneShortPressOnASilentBackendIsNotLost()
    {
        // Active backend last spoke at 1000 and then went quiet; the candidate
        // pressed at 1101, released at 1141, and never pressed again.
        const qint64 pressed = 1101;
        const qint64 resolve = pressed + ControllerArbitration::BackendCandidateConfirmMs;
        QVERIFY(ControllerArbitration::heldPressSurvives(pressed, 1000, resolve));
        // Nothing else could have promoted it: the silence threshold is far
        // away and no second candidate event exists to confirm a run.
        QVERIFY(!ControllerArbitration::backendMayTakeOver(true, 1000, resolve));
    }

    void heldPressIsDroppedWhenTheActiveBackendAnswers()
    {
        // A mirror: the active backend produced its own control after the
        // candidate's press, so that press was an echo and must not be
        // delivered — that would be the double-action bug all over again.
        const qint64 pressed = 1101;
        QVERIFY(!ControllerArbitration::heldPressSurvives(pressed, 1150, 1351));
        // Same instant counts as the active backend still being alive.
        QVERIFY(!ControllerArbitration::heldPressSurvives(pressed, pressed, 1351));
    }

    void trailingMirrorOfASingleTapNeverSurvives()
    {
        // The 0.7.4 regression pin (field-reported: "press once, the list
        // jumps two rows"). A mirror trails the original: XInput delivered
        // the tap at 1000, the WinMM view of the same pad re-reported it at
        // 1004, and the user pressed nothing else. The active backend spoke
        // BEFORE the candidate press — the answers-after check cannot catch
        // it — so the born-inside-the-mirror-window check must.
        QVERIFY(!ControllerArbitration::heldPressSurvives(1004, 1000, 1254));
        // Inclusive up to the window edge, matching backendMayTakeOver.
        QVERIFY(!ControllerArbitration::heldPressSurvives(
            1000 + ControllerArbitration::BackendDuplicateWindowMs, 1000, 1400));
        // One ms past the window is the genuine-failover territory the
        // oneShortPressOnASilentBackendIsNotLost case protects.
        QVERIFY(ControllerArbitration::heldPressSurvives(
            1001 + ControllerArbitration::BackendDuplicateWindowMs, 1000,
            1001 + ControllerArbitration::BackendDuplicateWindowMs
                + ControllerArbitration::BackendCandidateConfirmMs));
    }

    void heldPressSurvivesWhenTheActiveBackendNeverSpoke()
    {
        // The "active never reported a control" sentinel is qint64 min; the
        // mirror-window guard must not overflow when subtracting across it.
        QVERIFY(ControllerArbitration::heldPressSurvives(
            1101, std::numeric_limits<qint64>::min(),
            1101 + ControllerArbitration::BackendCandidateConfirmMs));
    }

    void heldPressIsNotDeliveredBeforeTheWindowCloses()
    {
        // Bounded wait: the press is delivered when the window has elapsed,
        // not a moment earlier.
        const qint64 pressed = 1101;
        QVERIFY(!ControllerArbitration::heldPressSurvives(
            pressed, 1000, pressed + ControllerArbitration::BackendCandidateConfirmMs - 1));
        QVERIFY(ControllerArbitration::heldPressSurvives(
            pressed, 1000, pressed + ControllerArbitration::BackendCandidateConfirmMs));
        // And the wait really is bounded — a quarter second, not a second.
        QVERIFY(ControllerArbitration::BackendCandidateConfirmMs
                < ControllerArbitration::BackendTakeoverSilenceMs);
    }

    void physicalSonyPadWinsImmediately()
    {
        QVERIFY(ControllerArbitration::sonyDeviceMayTakeOver(
            true, 3, 1, 20, 19, 1000));
    }

    void lowerPrioritySonyPathWaitsForRealIdle()
    {
        QVERIFY(!ControllerArbitration::sonyDeviceMayTakeOver(
            true, 1, 3, 1100, 100, 1000));
        QVERIFY(ControllerArbitration::sonyDeviceMayTakeOver(
            true, 1, 3, 1101, 100, 1000));
    }

    void idleTrafficCannotStealActiveSonyPad()
    {
        QVERIFY(!ControllerArbitration::sonyDeviceMayTakeOver(
            false, 3, 1, 5000, 0, 1000));
    }
};

QTEST_APPLESS_MAIN(ControllerArbitrationTest)
#include "tst_controllerarbitration.moc"
