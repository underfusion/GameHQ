// Gesture timing: clamping, and the one-release migration off the old
// `input.share_hold_ms` key.
//
// The migration is override-only, and that is the part worth pinning down. If
// it wrote a value for everyone, every user's config would silently freeze
// today's default forever and a later change to the shipped hold time would
// never reach them.

#include "input/GestureTiming.h"

#include <QTest>

class GestureTimingTest : public QObject
{
    Q_OBJECT

private slots:
    void holdIsClampedToAUsableRange()
    {
        QCOMPARE(GestureTiming::clampHoldMs(2000), 2000);
        // Below the floor a "hold" is indistinguishable from a tap.
        QCOMPARE(GestureTiming::clampHoldMs(10), GestureTiming::kMinHoldMs);
        QCOMPARE(GestureTiming::clampHoldMs(-5000), GestureTiming::kMinHoldMs);
        QCOMPARE(GestureTiming::clampHoldMs(999999), GestureTiming::kMaxHoldMs);
    }

    void multiTapAndChordWindowsAreClamped()
    {
        QCOMPARE(GestureTiming::clampMultiTapIntervalMs(300), 300);
        QCOMPARE(GestureTiming::clampMultiTapIntervalMs(0), GestureTiming::kMinMultiTapMs);
        QCOMPARE(GestureTiming::clampMultiTapIntervalMs(99999), GestureTiming::kMaxMultiTapMs);

        QCOMPARE(GestureTiming::clampChordWindowMs(300), 300);
        QCOMPARE(GestureTiming::clampChordWindowMs(1), GestureTiming::kMinChordWindowMs);
        // The chord window is latency a shared button pays, so it is capped
        // harder than the hold.
        QCOMPARE(GestureTiming::clampChordWindowMs(5000), GestureTiming::kMaxChordWindowMs);
        QVERIFY(GestureTiming::kMaxChordWindowMs < GestureTiming::kMaxHoldMs);
    }

    void aRealLegacyOverrideIsCarriedAcrossOnce()
    {
        const auto migrated = GestureTiming::migratedHoldMs(
            /*hasNewOverride=*/false, /*hasLegacyOverride=*/true, 1500);
        QVERIFY(migrated.has_value());
        QCOMPARE(*migrated, 1500);
    }

    void aLegacyValueOutsideTheRangeIsClampedOnTheWayIn()
    {
        const auto migrated = GestureTiming::migratedHoldMs(false, true, 50);
        QVERIFY(migrated.has_value());
        QCOMPARE(*migrated, GestureTiming::kMinHoldMs);
    }

    void nothingIsWrittenForAUserWhoNeverChangedIt()
    {
        // No override under either name: writing one would freeze today's
        // default into their config and hide any future change to it.
        QVERIFY(!GestureTiming::migratedHoldMs(false, false, 2000).has_value());
    }

    void anExplicitNewValueWins()
    {
        // The user has already answered this question under the new name.
        QVERIFY(!GestureTiming::migratedHoldMs(true, true, 1000).has_value());
    }

    void timingIsBuiltFromClampedValues()
    {
        const auto timing = GestureTiming::fromValues(99999, 1, 99999);
        QCOMPARE(timing.defaultHoldMs, GestureTiming::kMaxHoldMs);
        QCOMPARE(timing.multiTapIntervalMs, GestureTiming::kMinMultiTapMs);
        QCOMPARE(timing.chordWindowMs, GestureTiming::kMaxChordWindowMs);
    }

    void theDescriptionCarriesAllThreeNumbers()
    {
        const QString text = GestureTiming::describe(GestureTiming::fromValues(1500, 250, 400));
        QVERIFY(text.contains(QStringLiteral("1500")));
        QVERIFY(text.contains(QStringLiteral("250")));
        QVERIFY(text.contains(QStringLiteral("400")));
    }
};

QTEST_MAIN(GestureTimingTest)
#include "tst_gesturetiming.moc"
