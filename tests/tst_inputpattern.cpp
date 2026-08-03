// The pattern recognizer's timing state machine, driven through a fake facts
// provider so no database, device or binding table is involved.
//
// Timings are deliberately small (a 40 ms multi-tap window) and every wait is a
// generous multiple of them: these tests assert ordering and counting, not the
// wall clock, so they must not become flaky on a loaded machine.

#include "input/InputPatternRecognizer.h"

#include <QSignalSpy>
#include <QTest>

using Facts = InputPatternRecognizer::TriggerFacts;
using Context = InputPatternRecognizer::Context;

namespace {

const QString kControl = QStringLiteral("gamepad.capture");
const QString kOther = QStringLiteral("gamepad.guide");

constexpr int kMultiTapMs = 40;
constexpr int kHoldMs = 60;
// Long enough that a pending window has certainly expired, short enough to keep
// the suite fast.
constexpr int kSettleMs = 200;

Context ctx(ActionCatalog::Scope primary = ActionCatalog::Scope::Global)
{
    return {QStringLiteral("controller"), QString(), primary, ActionCatalog::Scope::Global};
}

int tapMask(std::initializer_list<int> counts)
{
    int mask = 0;
    for (int count : counts)
        mask |= 1 << count;
    return mask;
}

} // namespace

class InputPatternTest : public QObject
{
    Q_OBJECT

private:
    InputPatternRecognizer* m_recognizer = nullptr;
    QSignalSpy* m_spy = nullptr;
    Facts m_facts;

    void setFacts(const Facts& facts)
    {
        m_facts = facts;
        m_recognizer->setFactsProvider([this](const Context&, const QString&) {
            return m_facts;
        });
    }

    // One tap: press then release, with no wait in between.
    void tap(const Context& context = ctx(), const QString& control = kControl)
    {
        m_recognizer->press(context, control);
        m_recognizer->release(context, control);
    }

    GestureSpec gestureAt(int index) const
    {
        return m_spy->at(index).at(2).value<GestureSpec>();
    }

    TriggerSpec triggerAt(int index) const
    {
        return m_spy->at(index).at(1).value<TriggerSpec>();
    }

private slots:
    void initTestCase()
    {
        qRegisterMetaType<InputPatternRecognizer::Context>();
        qRegisterMetaType<TriggerSpec>();
        qRegisterMetaType<GestureSpec>();
    }

    void init()
    {
        m_recognizer = new InputPatternRecognizer(this);
        m_recognizer->setTiming({kMultiTapMs, kMultiTapMs, 500});
        m_spy = new QSignalSpy(m_recognizer, &InputPatternRecognizer::recognized);
        setFacts({});
    }

    void cleanup()
    {
        delete m_spy;
        m_spy = nullptr;
        delete m_recognizer;
        m_recognizer = nullptr;
    }

    // ------------------------------------------------------------ press

    void pressFiresOnTheDownEdge()
    {
        Facts facts;
        facts.hasPress = true;
        setFacts(facts);

        m_recognizer->press(ctx(), kControl);
        QCOMPARE(m_spy->count(), 1);
        QCOMPARE(gestureAt(0), GestureSpec::press());
        QCOMPARE(triggerAt(0), TriggerSpec::single(kControl));

        // Releasing a press-only control adds nothing.
        m_recognizer->release(ctx(), kControl);
        QTest::qWait(kSettleMs);
        QCOMPARE(m_spy->count(), 1);
    }

    void anUnboundControlIsNotConsumed()
    {
        QVERIFY(!m_recognizer->press(ctx(), kControl));
        QCOMPARE(m_spy->count(), 0);
    }

    // -------------------------------------------------------------- taps

    void aLoneSingleTapFiresImmediately()
    {
        Facts facts;
        facts.tapCountMask = tapMask({1});
        setFacts(facts);

        tap();
        // No higher count is bound, so there is nothing to wait for.
        QCOMPARE(m_spy->count(), 1);
        QCOMPARE(gestureAt(0), GestureSpec::tap(1));
    }

    void exactCountDispatch_data()
    {
        QTest::addColumn<int>("bound");
        QTest::addColumn<int>("performed");

        QTest::newRow("x1 alone") << 1 << 1;
        QTest::newRow("x2 alone") << 2 << 2;
        QTest::newRow("x3 alone") << 3 << 3;
    }

    void exactCountDispatch()
    {
        QFETCH(int, bound);
        QFETCH(int, performed);
        Facts facts;
        facts.tapCountMask = tapMask({bound});
        setFacts(facts);

        for (int i = 0; i < performed; ++i)
            tap();
        QTest::qWait(kSettleMs);

        QCOMPARE(m_spy->count(), 1);
        QCOMPARE(gestureAt(0), GestureSpec::tap(performed));
    }

    void threeTapsFireOnlyTheTripleTapNeverTheWayUp()
    {
        // The whole point of exact-count dispatch: x1, x2 and x3 all bound on
        // one control, three taps must produce one action, not three.
        Facts facts;
        facts.tapCountMask = tapMask({1, 2, 3});
        setFacts(facts);

        tap();
        tap();
        tap();
        QTest::qWait(kSettleMs);

        QCOMPARE(m_spy->count(), 1);
        QCOMPARE(gestureAt(0), GestureSpec::tap(3));
    }

    void aLowerCountWaitsOnlyWhileAHigherOneIsBound()
    {
        Facts facts;
        facts.tapCountMask = tapMask({1, 2});
        setFacts(facts);

        tap();
        // x2 exists, so x1 must hold back rather than fire on the release edge.
        QCOMPARE(m_spy->count(), 0);
        QTest::qWait(kSettleMs);
        QCOMPARE(m_spy->count(), 1);
        QCOMPARE(gestureAt(0), GestureSpec::tap(1));

        // Reaching the highest bound count fires without waiting at all.
        m_spy->clear();
        tap();
        tap();
        QCOMPARE(m_spy->count(), 1);
        QCOMPARE(gestureAt(0), GestureSpec::tap(2));
    }

    void anUnboundCountInTheMiddleFiresNothing()
    {
        // x1 and x3 bound, x2 not: two taps mean neither of them. Falling back
        // to x1 would fire an action the user did not ask for.
        Facts facts;
        facts.tapCountMask = tapMask({1, 3});
        setFacts(facts);

        tap();
        tap();
        QTest::qWait(kSettleMs);
        QCOMPARE(m_spy->count(), 0);

        // The sequence really did reset — a later single tap still works.
        tap();
        QTest::qWait(kSettleMs);
        QCOMPARE(m_spy->count(), 1);
        QCOMPARE(gestureAt(0), GestureSpec::tap(1));
    }

    void tapsSeparatedByMoreThanTheIntervalAreSeparateSequences()
    {
        Facts facts;
        facts.tapCountMask = tapMask({1, 2});
        setFacts(facts);

        tap();
        QTest::qWait(kSettleMs);
        tap();
        QTest::qWait(kSettleMs);

        QCOMPARE(m_spy->count(), 2);
        QCOMPARE(gestureAt(0), GestureSpec::tap(1));
        QCOMPARE(gestureAt(1), GestureSpec::tap(1));
    }

    // ------------------------------------------------------------- holds

    void holdFiresAtItsThresholdAndConsumesTheTap()
    {
        Facts facts;
        facts.tapCountMask = tapMask({1});
        facts.holdThresholdsMs = {kHoldMs};
        setFacts(facts);

        m_recognizer->press(ctx(), kControl);
        QTest::qWait(kHoldMs * 3);
        QCOMPARE(m_spy->count(), 1);
        QCOMPARE(gestureAt(0), GestureSpec::hold(kHoldMs));

        // Releasing after a hold must not also produce a tap — that would
        // screenshot every time the user saved a replay.
        m_recognizer->release(ctx(), kControl);
        QTest::qWait(kSettleMs);
        QCOMPARE(m_spy->count(), 1);
    }

    void aQuickPressStillTapsWhileAHoldIsBound()
    {
        Facts facts;
        facts.tapCountMask = tapMask({1});
        facts.holdThresholdsMs = {kHoldMs};
        setFacts(facts);

        tap();
        QTest::qWait(kSettleMs);
        QCOMPARE(m_spy->count(), 1);
        QCOMPARE(gestureAt(0), GestureSpec::tap(1));
    }

    void twoHoldsOnOneControlEachFireOnceAtTheirOwnThreshold()
    {
        Facts facts;
        facts.holdThresholdsMs = {kHoldMs, kHoldMs * 3};
        setFacts(facts);

        m_recognizer->press(ctx(), kControl);
        QTest::qWait(kHoldMs * 2);
        QCOMPARE(m_spy->count(), 1);
        QCOMPARE(gestureAt(0), GestureSpec::hold(kHoldMs));

        QTest::qWait(kHoldMs * 3);
        QCOMPARE(m_spy->count(), 2);
        QCOMPARE(gestureAt(1), GestureSpec::hold(kHoldMs * 3));

        m_recognizer->release(ctx(), kControl);
        QTest::qWait(kSettleMs);
        QCOMPARE(m_spy->count(), 2);
    }

    void holdOnTheSecondTapIsTimedFromThatPress()
    {
        // Holding down the second tap of a double tap is a hold, not a tap that
        // happened to take a long time.
        Facts facts;
        facts.tapCountMask = tapMask({1, 2});
        facts.holdThresholdsMs = {kHoldMs};
        setFacts(facts);

        tap();
        m_recognizer->press(ctx(), kControl);
        QTest::qWait(kHoldMs * 3);
        m_recognizer->release(ctx(), kControl);
        QTest::qWait(kSettleMs);

        QCOMPARE(m_spy->count(), 1);
        QCOMPARE(gestureAt(0), GestureSpec::hold(kHoldMs));
    }

    // ------------------------------------------------------ cancellation

    void invalidateCancelsAPendingTapSequence()
    {
        // A backend switch, a disconnect or a binding reload: whatever the
        // reason, the delayed action must not land in the new world.
        Facts facts;
        facts.tapCountMask = tapMask({1, 2});
        setFacts(facts);

        tap();
        QCOMPARE(m_spy->count(), 0);
        m_recognizer->invalidate();
        QTest::qWait(kSettleMs);
        QCOMPARE(m_spy->count(), 0);
    }

    void invalidateCancelsAPendingHold()
    {
        Facts facts;
        facts.holdThresholdsMs = {kHoldMs};
        setFacts(facts);

        m_recognizer->press(ctx(), kControl);
        m_recognizer->invalidate();
        QTest::qWait(kHoldMs * 4);
        QCOMPARE(m_spy->count(), 0);
    }

    void changingTheTimingCancelsWhatWasSnapshotted()
    {
        Facts facts;
        facts.tapCountMask = tapMask({1, 2});
        setFacts(facts);

        tap();
        m_recognizer->setTiming({kMultiTapMs * 4, kMultiTapMs, 500});
        QTest::qWait(kSettleMs);
        QCOMPARE(m_spy->count(), 0);
    }

    void aScopeChangeBetweenTapsStartsAFreshPattern()
    {
        // First tap in the gallery, overlay opens, second tap: the pending
        // gallery action must not fire, and the new press belongs to the
        // overlay.
        Facts facts;
        facts.tapCountMask = tapMask({1, 2});
        setFacts(facts);

        tap(ctx(ActionCatalog::Scope::Desktop));
        QCOMPARE(m_spy->count(), 0);

        const Context overlay = ctx(ActionCatalog::Scope::Overlay);
        tap(overlay);
        QTest::qWait(kSettleMs);

        // One tap in each context, never a double tap spanning the two.
        QCOMPARE(m_spy->count(), 1);
        QCOMPARE(gestureAt(0), GestureSpec::tap(1));
        QCOMPARE(m_spy->at(0).at(0).value<Context>().primaryScope,
                 ActionCatalog::Scope::Overlay);
    }

    void aPatternKeepsTheContextItStartedIn()
    {
        Facts facts;
        facts.tapCountMask = tapMask({1, 2});
        setFacts(facts);

        const Context desktop = ctx(ActionCatalog::Scope::Desktop);
        tap(desktop);
        QTest::qWait(kSettleMs);

        QCOMPARE(m_spy->count(), 1);
        QCOMPARE(m_spy->at(0).at(0).value<Context>().primaryScope,
                 ActionCatalog::Scope::Desktop);
    }

    void separateControlsKeepSeparateCounters()
    {
        Facts facts;
        facts.tapCountMask = tapMask({1, 2});
        setFacts(facts);

        tap(ctx(), kControl);
        tap(ctx(), kOther);
        QTest::qWait(kSettleMs);

        QCOMPARE(m_spy->count(), 2);
        QCOMPARE(gestureAt(0), GestureSpec::tap(1));
        QCOMPARE(gestureAt(1), GestureSpec::tap(1));
        // Two independent timers expire in the same event-loop pass, so which
        // one is delivered first is not something to pin down here. What
        // matters is that each control produced its own tap.
        const QStringList fired{triggerAt(0).firstControl(), triggerAt(1).firstControl()};
        QVERIFY(fired.contains(kControl));
        QVERIFY(fired.contains(kOther));
    }

    void aReleaseWithoutAPressIsIgnored()
    {
        Facts facts;
        facts.tapCountMask = tapMask({1});
        setFacts(facts);

        QVERIFY(!m_recognizer->release(ctx(), kControl));
        QCOMPARE(m_spy->count(), 0);
    }

    // ------------------------------------------------------------ chords

    void chordFiresWhenThePartnerArrivesInsideTheWindow()
    {
        // Hold the first control, press the second: one chord action, and
        // neither constituent acts on its own.
        m_recognizer->setFactsProvider([](const Context&, const QString& control) {
            Facts facts;
            if (control == kControl) {
                facts.chordPartners = {kOther};
                facts.tapCountMask = tapMask({1});
            }
            return facts;
        });

        m_recognizer->press(ctx(), kControl);
        QCOMPARE(m_spy->count(), 0);          // the first control holds back
        m_recognizer->press(ctx(), kOther);
        QCOMPARE(m_spy->count(), 1);
        QCOMPARE(triggerAt(0), TriggerSpec::orderedChord(kControl, kOther));
        QCOMPARE(gestureAt(0), GestureSpec::press());

        m_recognizer->release(ctx(), kOther);
        m_recognizer->release(ctx(), kControl);
        QTest::qWait(kSettleMs);
        // The chord consumed both presses: no stray tap from either control.
        QCOMPARE(m_spy->count(), 1);
    }

    void anExpiredChordReplaysTheConstituentAsAHold()
    {
        // Nobody pressed the partner. The first control was held the whole
        // time, so its hold is due at the timeout, timed from the physical
        // press — not from the moment the window gave up.
        m_recognizer->setFactsProvider([](const Context&, const QString& control) {
            Facts facts;
            if (control == kControl) {
                facts.chordPartners = {kOther};
                facts.holdThresholdsMs = {kMultiTapMs};  // shorter than the window
            }
            return facts;
        });

        m_recognizer->press(ctx(), kControl);
        QTest::qWait(kSettleMs);
        QCOMPARE(m_spy->count(), 1);
        QCOMPARE(gestureAt(0), GestureSpec::hold(kMultiTapMs));
        QCOMPARE(triggerAt(0), TriggerSpec::single(kControl));
    }

    void releasingTheFirstControlEarlyStillTaps()
    {
        // A quick tap on a button that also starts a chord must stay a tap —
        // and must not be delayed into a hold by the chord window.
        m_recognizer->setFactsProvider([](const Context&, const QString& control) {
            Facts facts;
            if (control == kControl) {
                facts.chordPartners = {kOther};
                facts.tapCountMask = tapMask({1});
                facts.holdThresholdsMs = {kMultiTapMs};
            }
            return facts;
        });

        m_recognizer->press(ctx(), kControl);
        m_recognizer->release(ctx(), kControl);
        QTest::qWait(kSettleMs);

        QCOMPARE(m_spy->count(), 1);
        QCOMPARE(gestureAt(0), GestureSpec::tap(1));
    }

    void aHoldLongerThanTheChordWindowStillFiresAtItsOwnThreshold()
    {
        m_recognizer->setFactsProvider([](const Context&, const QString& control) {
            Facts facts;
            if (control == kControl) {
                facts.chordPartners = {kOther};
                facts.holdThresholdsMs = {kMultiTapMs * 3};  // longer than the window
            }
            return facts;
        });

        m_recognizer->press(ctx(), kControl);
        QTest::qWait(kMultiTapMs * 2);        // window closed, threshold not reached
        QCOMPARE(m_spy->count(), 0);
        QTest::qWait(kSettleMs);
        QCOMPARE(m_spy->count(), 1);
        QCOMPARE(gestureAt(0), GestureSpec::hold(kMultiTapMs * 3));
    }

    void aCompletedChordSuppressesTheFirstControlsHold()
    {
        m_recognizer->setFactsProvider([](const Context&, const QString& control) {
            Facts facts;
            if (control == kControl) {
                facts.chordPartners = {kOther};
                facts.holdThresholdsMs = {kMultiTapMs};
            }
            return facts;
        });

        m_recognizer->press(ctx(), kControl);
        m_recognizer->press(ctx(), kOther);   // completes before the hold is due
        QTest::qWait(kSettleMs);

        QCOMPARE(m_spy->count(), 1);
        QCOMPARE(triggerAt(0), TriggerSpec::orderedChord(kControl, kOther));
    }

    void theSecondControlSelectsAmongSeveralCandidates()
    {
        const QString third = QStringLiteral("gamepad.face_north");
        m_recognizer->setFactsProvider([&third](const Context&, const QString& control) {
            Facts facts;
            if (control == kControl)
                facts.chordPartners = {kOther, third};
            return facts;
        });

        m_recognizer->press(ctx(), kControl);
        m_recognizer->press(ctx(), third);
        QCOMPARE(m_spy->count(), 1);
        QCOMPARE(triggerAt(0), TriggerSpec::orderedChord(kControl, third));
    }

    void anUnmatchedSecondControlKeepsItsOwnBehaviour()
    {
        const QString stranger = QStringLiteral("gamepad.face_west");
        m_recognizer->setFactsProvider([&stranger](const Context&, const QString& control) {
            Facts facts;
            if (control == kControl)
                facts.chordPartners = {kOther};
            if (control == stranger)
                facts.tapCountMask = tapMask({1});
            return facts;
        });

        m_recognizer->press(ctx(), kControl);
        m_recognizer->press(ctx(), stranger);
        m_recognizer->release(ctx(), stranger);
        QTest::qWait(kSettleMs);

        // The stranger tapped as usual; the candidate expired with nothing of
        // its own to replay.
        QCOMPARE(m_spy->count(), 1);
        QCOMPARE(gestureAt(0), GestureSpec::tap(1));
        QCOMPARE(triggerAt(0), TriggerSpec::single(stranger));
    }

    void aRepeatedFirstControlPressDoesNotOpenASecondCandidate()
    {
        // A mirrored backend (physical DualSense plus a virtual pad) can repeat
        // an edge. It must not restart the window or fire the chord twice.
        m_recognizer->setFactsProvider([](const Context&, const QString& control) {
            Facts facts;
            if (control == kControl)
                facts.chordPartners = {kOther};
            return facts;
        });

        m_recognizer->press(ctx(), kControl);
        m_recognizer->press(ctx(), kControl);
        m_recognizer->press(ctx(), kOther);
        m_recognizer->press(ctx(), kOther);
        QTest::qWait(kSettleMs);

        QCOMPARE(m_spy->count(), 1);
        QCOMPARE(triggerAt(0), TriggerSpec::orderedChord(kControl, kOther));
    }

    void invalidateCancelsAnOpenChordCandidate()
    {
        m_recognizer->setFactsProvider([](const Context&, const QString& control) {
            Facts facts;
            if (control == kControl) {
                facts.chordPartners = {kOther};
                facts.holdThresholdsMs = {kMultiTapMs};
            }
            return facts;
        });

        m_recognizer->press(ctx(), kControl);
        m_recognizer->invalidate();
        QTest::qWait(kSettleMs);
        // Neither the chord nor its fallback may land after a backend switch.
        QCOMPARE(m_spy->count(), 0);
    }

    void aChordDoesNotCompleteAcrossAContextChange()
    {
        m_recognizer->setFactsProvider([](const Context&, const QString& control) {
            Facts facts;
            if (control == kControl)
                facts.chordPartners = {kOther};
            return facts;
        });

        m_recognizer->press(ctx(ActionCatalog::Scope::Desktop), kControl);
        m_recognizer->press(ctx(ActionCatalog::Scope::Overlay), kOther);
        QTest::qWait(kSettleMs);
        QCOMPARE(m_spy->count(), 0);
    }

    void repeatedPressesDoNotAllocateNewState()
    {
        // The flood-hardened input path must stay allocation-free once a button
        // is known. State is per (context, control) and reused; the observable
        // proxy is that a thousand cycles behave exactly like one.
        Facts facts;
        facts.tapCountMask = tapMask({1});
        setFacts(facts);

        for (int i = 0; i < 1000; ++i)
            tap();
        QCOMPARE(m_spy->count(), 1000);
    }
};

QTEST_MAIN(InputPatternTest)
#include "tst_inputpattern.moc"
