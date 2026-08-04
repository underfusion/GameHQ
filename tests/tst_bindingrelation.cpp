#include "input/ActionCatalog.h"
#include "input/BindingRelation.h"
#include "input/BindingResolver.h"
#include "input/ControlId.h"

#include <QTest>

using Kind = BindingRelation::Kind;
using Scope = ActionCatalog::Scope;

namespace {

// Builds a controller binding for an existing catalog action. Every action id
// used below is a real catalog entry — classify() resolves scopes through the
// catalog, so invented ids would silently collapse to Kind::None.
BindingResolver::Binding controller(const char* actionId, const QString& trigger,
                                    const GestureSpec& gesture = GestureSpec::press(),
                                    const QString& profile = {}, int slot = 1)
{
    return {QStringLiteral("controller"), profile, QString::fromLatin1(actionId), slot,
            trigger, gesture.activationCode(), gesture.holdMs, false, gesture.tapCount};
}

// An ordered two-button chord bound to an action. Chords are Press-only in v1.
BindingResolver::Binding chord(const char* actionId, const QString& first,
                               const QString& second, int slot = 1,
                               const QString& profile = {})
{
    return {QStringLiteral("controller"), profile, QString::fromLatin1(actionId), slot,
            TriggerSpec::orderedChord(first, second).serialize(),
            QStringLiteral("press"), 0, false, 1};
}

// Shorthands so the matrix rows below read like the gestures they describe.
const GestureSpec kPress = GestureSpec::press();
const GestureSpec kTap = GestureSpec::tap(1);
const GestureSpec kDoubleTap = GestureSpec::tap(2);
const GestureSpec kTripleTap = GestureSpec::tap(3);
const GestureSpec kHold = GestureSpec::hold(0);

} // namespace

class BindingRelationTest : public QObject
{
    Q_OBJECT

private slots:
    void scopeOverlapMatrix_data()
    {
        QTest::addColumn<Scope>("left");
        QTest::addColumn<Scope>("right");
        QTest::addColumn<bool>("overlaps");

        QTest::newRow("global/global")     << Scope::Global   << Scope::Global   << true;
        QTest::newRow("global/overlay")    << Scope::Global   << Scope::Overlay  << true;
        QTest::newRow("global/desktop")    << Scope::Global   << Scope::Desktop  << true;
        QTest::newRow("global/playback")   << Scope::Global   << Scope::Playback << true;
        QTest::newRow("overlay/overlay")   << Scope::Overlay  << Scope::Overlay  << true;
        QTest::newRow("desktop/desktop")   << Scope::Desktop  << Scope::Desktop  << true;
        QTest::newRow("playback/playback") << Scope::Playback << Scope::Playback << true;
        // Two different contextual scopes are a resolver priority chain, not a
        // simultaneous dispatch: primary wins and the fallback is consulted only
        // when primary matched nothing. D-pad Left ships as both
        // playback.seek_back and desktop.navigate_left for exactly this reason.
        QTest::newRow("overlay/playback")  << Scope::Overlay  << Scope::Playback << false;
        QTest::newRow("desktop/playback")  << Scope::Desktop  << Scope::Playback << false;
        QTest::newRow("overlay/desktop")   << Scope::Overlay  << Scope::Desktop  << false;
    }

    void scopeOverlapMatrix()
    {
        QFETCH(Scope, left);
        QFETCH(Scope, right);
        QFETCH(bool, overlaps);
        QCOMPARE(BindingRelation::scopesOverlap(left, right), overlaps);
        // The relation is symmetric; a one-sided answer would make classify()
        // depend on argument order.
        QCOMPARE(BindingRelation::scopesOverlap(right, left), overlaps);
    }

    void gestureCompatibilityMatrix_data()
    {
        QTest::addColumn<GestureSpec>("left");
        QTest::addColumn<GestureSpec>("right");
        QTest::addColumn<bool>("compatible");

        QTest::newRow("tap+hold")          << kTap        << kHold       << true;
        QTest::newRow("tap x1+tap x2")     << kTap        << kDoubleTap  << true;
        QTest::newRow("tap x1+tap x3")     << kTap        << kTripleTap  << true;
        QTest::newRow("tap x2+tap x3")     << kDoubleTap  << kTripleTap  << true;
        QTest::newRow("hold+tap x2")       << kHold       << kDoubleTap  << true;
        QTest::newRow("tap+tap")           << kTap        << kTap        << false;
        QTest::newRow("tap x2+tap x2")     << kDoubleTap  << kDoubleTap  << false;
        // Two holds are never separated in time: the shorter one always wins.
        QTest::newRow("hold+hold")         << kHold       << kHold       << false;
        QTest::newRow("hold+longer hold")  << kHold       << GestureSpec::hold(3000) << false;
        // press fires on the down edge, before any timed gesture is known.
        QTest::newRow("press+tap")         << kPress      << kTap        << false;
        QTest::newRow("press+hold")        << kPress      << kHold       << false;
        QTest::newRow("press+tap x2")      << kPress      << kDoubleTap  << false;
        QTest::newRow("press+press")       << kPress      << kPress      << false;
    }

    void gestureCompatibilityMatrix()
    {
        QFETCH(GestureSpec, left);
        QFETCH(GestureSpec, right);
        QFETCH(bool, compatible);
        QCOMPARE(BindingRelation::gesturesCompatible(left, right), compatible);
        QCOMPARE(BindingRelation::gesturesCompatible(right, left), compatible);
    }

    // ------------------------------------------------- chords and tap counts

    void tapCountsShareOneButton()
    {
        // x1 and x2 on one control are separated in time, so they share rather
        // than collide — the whole point of exact-count dispatch.
        QCOMPARE(BindingRelation::classify(
                     controller("global.screenshot", ControlId::Capture, kTap),
                     controller("global.toggle_overlay", ControlId::Capture, kDoubleTap)),
                 Kind::SharedGesture);
        QCOMPARE(BindingRelation::classify(
                     controller("global.screenshot", ControlId::Capture, kDoubleTap),
                     controller("global.toggle_overlay", ControlId::Capture, kTripleTap)),
                 Kind::SharedGesture);
    }

    void theSameTapCountOnTwoActionsIsAHardConflict()
    {
        QCOMPARE(BindingRelation::classify(
                     controller("global.screenshot", ControlId::Capture, kDoubleTap),
                     controller("global.toggle_overlay", ControlId::Capture, kDoubleTap)),
                 Kind::HardConflict);
    }

    void aLowerTapCountCarriesTheDelayNotice()
    {
        const auto single = controller("global.screenshot", ControlId::Capture, kTap);
        const auto twice = controller("global.toggle_overlay", ControlId::Capture, kDoubleTap);
        QCOMPARE(BindingRelation::noticeFor(single, twice),
                 BindingRelation::Notice::HigherTapCountDelay);
        // Symmetric: the pair is what carries the delay, not one side of it.
        QCOMPARE(BindingRelation::noticeFor(twice, single),
                 BindingRelation::Notice::HigherTapCountDelay);
        QVERIFY(BindingRelation::noticeText(BindingRelation::Notice::HigherTapCountDelay,
                                            300, 250)
                    .contains(QStringLiteral("250")));
    }

    void tapAndHoldCarryNoDelayNotice()
    {
        // Tap and hold are told apart by how long the button is down; neither
        // one waits on the other.
        QCOMPARE(BindingRelation::noticeFor(
                     controller("global.screenshot", ControlId::Capture, kTap),
                     controller("global.save_replay", ControlId::Capture, kHold)),
                 BindingRelation::Notice::None);
    }

    void triggersWithNoControlInCommonNeverCompete()
    {
        QCOMPARE(BindingRelation::classify(
                     controller("global.screenshot", ControlId::Capture, kTap),
                     controller("global.toggle_overlay", ControlId::Guide, kTap)),
                 Kind::None);
        QCOMPARE(BindingRelation::classify(
                     chord("global.screenshot", ControlId::Capture, ControlId::Guide),
                     controller("desktop.favorite", ControlId::FaceNorth, kPress)),
                 Kind::None);
    }

    void theSameChordTwiceIsAHardConflict()
    {
        QCOMPARE(BindingRelation::classify(
                     chord("global.screenshot", ControlId::Capture, ControlId::Guide),
                     chord("global.save_replay", ControlId::Capture, ControlId::Guide)),
                 Kind::HardConflict);
    }

    void twoChordsSharingAFirstControlCoexist()
    {
        // View+Guide and View+X: the second control is what selects between
        // them, so both can live on the same first button.
        QCOMPARE(BindingRelation::classify(
                     chord("global.screenshot", ControlId::Capture, ControlId::Guide),
                     chord("global.save_replay", ControlId::Capture, ControlId::FaceWest)),
                 Kind::None);
    }

    void anImmediatePressOnAChordsFirstControlIsAHardConflict()
    {
        // Frozen rule: the first control has to be holdable. A press fires on
        // the down edge, before the combination could ever form.
        QCOMPARE(BindingRelation::classify(
                     chord("global.screenshot", ControlId::Capture, ControlId::Guide),
                     controller("global.toggle_overlay", ControlId::Capture, kPress)),
                 Kind::HardConflict);
    }

    void aRepeatingNavigationBindingOnAChordsFirstControlIsAHardConflict()
    {
        // D-pad Left repeats while held, so holding it to reach the second
        // button would scroll the gallery on the way. No delay-notice escape.
        QCOMPARE(BindingRelation::classify(
                     chord("overlay.favorite", ControlId::DpadLeft, ControlId::Guide),
                     controller("overlay.navigate_left", ControlId::DpadLeft, kPress)),
                 Kind::HardConflict);
        // Also when the navigation binding is a tap rather than a press: what
        // makes it fatal is that it repeats, not how it starts.
        QCOMPARE(BindingRelation::classify(
                     chord("overlay.favorite", ControlId::DpadLeft, ControlId::Guide),
                     controller("overlay.navigate_left", ControlId::DpadLeft, kTap)),
                 Kind::HardConflict);
    }

    void aTapOnAChordsFirstControlIsAllowedButCarriesTheChordDelayNotice()
    {
        const auto combination = chord("global.save_replay", ControlId::Capture,
                                       ControlId::Guide);
        const auto tapBinding = controller("global.screenshot", ControlId::Capture, kTap);
        QCOMPARE(BindingRelation::classify(combination, tapBinding), Kind::SharedGesture);
        QCOMPARE(BindingRelation::noticeFor(combination, tapBinding),
                 BindingRelation::Notice::ChordStartDelay);
        QCOMPARE(BindingRelation::noticeFor(tapBinding, combination),
                 BindingRelation::Notice::ChordStartDelay);
        // The number in the copy is the number the runtime actually waits.
        QVERIFY(BindingRelation::noticeText(BindingRelation::Notice::ChordStartDelay, 300, 250)
                    .contains(QStringLiteral("300")));
    }

    void aBindingOnAChordsSecondControlSharesRatherThanConflicts()
    {
        // Guide keeps toggling the overlay; it only loses that press when the
        // chord actually completes, which the user asked for by holding View.
        QCOMPARE(BindingRelation::classify(
                     chord("global.save_replay", ControlId::Capture, ControlId::Guide),
                     controller("global.toggle_overlay", ControlId::Guide, kPress)),
                 Kind::SharedGesture);
        QCOMPARE(BindingRelation::noticeFor(
                     chord("global.save_replay", ControlId::Capture, ControlId::Guide),
                     controller("global.toggle_overlay", ControlId::Guide, kPress)),
                 BindingRelation::Notice::None);
    }

    void chordsInNonOverlappingScopesNeverCompete()
    {
        QCOMPARE(BindingRelation::classify(
                     chord("overlay.favorite", ControlId::Capture, ControlId::Guide),
                     chord("desktop.favorite", ControlId::Capture, ControlId::Guide)),
                 Kind::None);
    }

    void identicalChordsOnOneActionAndProfileAreRedundant()
    {
        QCOMPARE(BindingRelation::classify(
                     chord("global.screenshot", ControlId::Capture, ControlId::Guide, 1),
                     chord("global.screenshot", ControlId::Capture, ControlId::Guide, 2)),
                 Kind::Redundant);
    }

    void secondarySlotsAreClassifiedLikePrimaryOnes()
    {
        // The matrix must consider both slots of every action: a secondary
        // assignment collides exactly as a primary one would.
        QCOMPARE(BindingRelation::classify(
                     controller("global.screenshot", ControlId::Capture, kDoubleTap, {}, 2),
                     controller("global.toggle_overlay", ControlId::Capture, kDoubleTap, {}, 1)),
                 Kind::HardConflict);
    }

    void differentDeviceGroupsNeverCompete()
    {
        BindingResolver::Binding key{QStringLiteral("keyboard"), {},
                                     QStringLiteral("global.screenshot"), 1,
                                     QStringLiteral("F12"), QStringLiteral("press"), 0, false, 1};
        BindingResolver::Binding pad = controller("global.save_replay", QStringLiteral("F12"));
        QCOMPARE(BindingRelation::classify(key, pad), Kind::None);
    }

    void differentTriggersNeverCompete()
    {
        // The same action reached from two different triggers is a deliberate
        // second slot, not redundancy. This is the case the old
        // scopesConflict() == (left == right) rule got wrong.
        BindingResolver::Binding a{QStringLiteral("keyboard"), {},
                                   QStringLiteral("global.screenshot"), 1,
                                   QStringLiteral("Ctrl+Shift+S"), QStringLiteral("press"), 0, false, 1};
        BindingResolver::Binding b{QStringLiteral("keyboard"), {},
                                   QStringLiteral("global.screenshot"), 2,
                                   QStringLiteral("F12"), QStringLiteral("press"), 0, false, 1};
        QCOMPARE(BindingRelation::classify(a, b), Kind::None);
    }

    void nonOverlappingScopesNeverCompete()
    {
        // Overlay and Desktop are never the active context at once, so the same
        // button meaning different things in each is correct by design.
        QCOMPARE(BindingRelation::classify(controller("overlay.favorite", ControlId::FaceNorth),
                                           controller("desktop.menu", ControlId::FaceNorth)),
                 Kind::None);
    }

    void declaredOverrideOutranksConflict()
    {
        // Same trigger, same gesture, different actions, overlapping scopes —
        // step 5 would call this a hard conflict, but the declared substitution
        // at step 2 wins first.
        const Kind kind = BindingRelation::classify(
            controller("playback.frame_grab", ControlId::Capture, kTap),
            controller("global.screenshot", ControlId::Capture, kTap));
        QCOMPARE(kind, Kind::ContextOverride);
        // Argument order must not change the verdict.
        QCOMPARE(BindingRelation::classify(
                     controller("global.screenshot", ControlId::Capture, kTap),
                     controller("playback.frame_grab", ControlId::Capture, kTap)),
                 Kind::ContextOverride);
    }

    void overrideIsScopedToItsGesture()
    {
        // The override is declared for tap only. Hold on the same button is a
        // different gesture pair and must classify as shared, not overridden.
        QCOMPARE(BindingRelation::classify(
                     controller("playback.frame_grab", ControlId::Capture, kTap),
                     controller("global.save_replay", ControlId::Capture, kHold)),
                 Kind::SharedGesture);
    }

    void redundantRequiresIdenticalTriggerAndGesture()
    {
        QCOMPARE(BindingRelation::classify(
                     controller("global.screenshot", ControlId::Capture, kTap, {}, 1),
                     controller("global.screenshot", ControlId::Capture, kTap, {}, 2)),
                 Kind::Redundant);
    }

    void deviceSpecificRowLayersRatherThanConflicts()
    {
        // A per-controller override of the same action is the override
        // mechanism working; the resolver merges them and one survives.
        QCOMPARE(BindingRelation::classify(
                     controller("global.screenshot", ControlId::Capture, kTap, {}),
                     controller("global.screenshot", ControlId::Capture, kTap,
                                QStringLiteral("054C:0CE6"))),
                 Kind::None);
    }

    void complementaryGesturesShareOneButton()
    {
        QCOMPARE(BindingRelation::classify(
                     controller("global.screenshot", ControlId::Capture, kTap),
                     controller("global.save_replay", ControlId::Capture, kHold)),
                 Kind::SharedGesture);
        QCOMPARE(BindingRelation::classify(
                     controller("global.screenshot", ControlId::Capture, kTap),
                     controller("global.toggle_overlay", ControlId::Capture, kDoubleTap)),
                 Kind::SharedGesture);
    }

    void sameGestureOnDifferentActionsIsHardConflict()
    {
        QCOMPARE(BindingRelation::classify(
                     controller("overlay.favorite", ControlId::FaceNorth),
                     controller("overlay.menu", ControlId::FaceNorth)),
                 Kind::HardConflict);
    }

    void editablePressAgainstTimedGestureRequiresConversion()
    {
        // Global Press can become Tap x1 so a triple-tap assignment can share
        // the control without deleting either action.
        QCOMPARE(BindingRelation::classify(
                      controller("global.toggle_overlay", ControlId::Menu, kPress),
                      controller("overlay.sidebar_toggle", ControlId::Menu, kTripleTap)),
                  Kind::ConversionRequired);
        QCOMPARE(BindingRelation::classify(
                     controller("overlay.sidebar_toggle", ControlId::Menu, kTripleTap),
                     controller("global.toggle_overlay", ControlId::Menu, kPress)),
                 Kind::ConversionRequired);
    }

    void pressAgainstSingleTapRemainsAHardConflict()
    {
        // Converting Press to Tap x1 would create the exact same gesture as the
        // requested assignment, so there is no compatible pair to offer.
        QCOMPARE(BindingRelation::classify(
                     controller("global.toggle_overlay", ControlId::Menu, kPress),
                     controller("overlay.sidebar_toggle", ControlId::Menu, kTap)),
                 Kind::HardConflict);
    }

    void repeatingAndFixedPressesNeverOfferConversion()
    {
        QCOMPARE(BindingRelation::classify(
                     controller("overlay.navigate_left", ControlId::DpadLeft, kPress),
                     controller("overlay.favorite", ControlId::DpadLeft, kTripleTap)),
                 Kind::HardConflict);
        QCOMPARE(BindingRelation::classify(
                     controller("overlay.back", ControlId::FaceEast, kPress),
                     controller("global.toggle_overlay", ControlId::FaceEast, kTripleTap)),
                 Kind::HardConflict);
    }

    void unboundRowsAreInert()
    {
        BindingResolver::Binding cleared = controller("overlay.menu", ControlId::FaceNorth);
        cleared.unbound = true;
        QCOMPARE(BindingRelation::classify(controller("overlay.favorite", ControlId::FaceNorth),
                                           cleared),
                 Kind::None);
    }

    void kindIdsAreStable()
    {
        QCOMPARE(BindingRelation::kindId(Kind::None), QStringLiteral("none"));
        QCOMPARE(BindingRelation::kindId(Kind::ContextOverride), QStringLiteral("context_override"));
        QCOMPARE(BindingRelation::kindId(Kind::Redundant), QStringLiteral("redundant"));
        QCOMPARE(BindingRelation::kindId(Kind::SharedGesture), QStringLiteral("shared_gesture"));
        QCOMPARE(BindingRelation::kindId(Kind::ConversionRequired),
                 QStringLiteral("conversion_required"));
        QCOMPARE(BindingRelation::kindId(Kind::HardConflict), QStringLiteral("hard_conflict"));
    }
};

QTEST_GUILESS_MAIN(BindingRelationTest)
#include "tst_bindingrelation.moc"
