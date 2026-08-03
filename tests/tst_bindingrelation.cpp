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
                                    const char* activation = "press",
                                    const QString& profile = {}, int slot = 1)
{
    return {QStringLiteral("controller"), profile, QString::fromLatin1(actionId), slot,
            trigger, QString::fromLatin1(activation), 0, false};
}

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
        QTest::addColumn<QString>("left");
        QTest::addColumn<QString>("right");
        QTest::addColumn<bool>("compatible");

        QTest::newRow("tap+hold")         << "tap"        << "hold"       << true;
        QTest::newRow("tap+double_tap")   << "tap"        << "double_tap" << true;
        QTest::newRow("hold+double_tap")  << "hold"       << "double_tap" << true;
        QTest::newRow("tap+tap")          << "tap"        << "tap"        << false;
        QTest::newRow("hold+hold")        << "hold"       << "hold"       << false;
        // press fires on the down edge, before any timed gesture is known.
        QTest::newRow("press+tap")        << "press"      << "tap"        << false;
        QTest::newRow("press+hold")       << "press"      << "hold"       << false;
        QTest::newRow("press+double_tap") << "press"      << "double_tap" << false;
        QTest::newRow("press+press")      << "press"      << "press"      << false;
        // Legacy rows stored before activations existed read back as empty.
        QTest::newRow("empty+tap")        << ""           << "tap"        << false;
    }

    void gestureCompatibilityMatrix()
    {
        QFETCH(QString, left);
        QFETCH(QString, right);
        QFETCH(bool, compatible);
        QCOMPARE(BindingRelation::gesturesCompatible(left, right), compatible);
        QCOMPARE(BindingRelation::gesturesCompatible(right, left), compatible);
    }

    void differentDeviceGroupsNeverCompete()
    {
        BindingResolver::Binding key{QStringLiteral("keyboard"), {},
                                     QStringLiteral("global.screenshot"), 1,
                                     QStringLiteral("F12"), QStringLiteral("press"), 0, false};
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
                                   QStringLiteral("Ctrl+Shift+S"), QStringLiteral("press"), 0, false};
        BindingResolver::Binding b{QStringLiteral("keyboard"), {},
                                   QStringLiteral("global.screenshot"), 2,
                                   QStringLiteral("F12"), QStringLiteral("press"), 0, false};
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
            controller("playback.frame_grab", ControlId::Capture, "tap"),
            controller("global.screenshot", ControlId::Capture, "tap"));
        QCOMPARE(kind, Kind::ContextOverride);
        // Argument order must not change the verdict.
        QCOMPARE(BindingRelation::classify(
                     controller("global.screenshot", ControlId::Capture, "tap"),
                     controller("playback.frame_grab", ControlId::Capture, "tap")),
                 Kind::ContextOverride);
    }

    void overrideIsScopedToItsGesture()
    {
        // The override is declared for tap only. Hold on the same button is a
        // different gesture pair and must classify as shared, not overridden.
        QCOMPARE(BindingRelation::classify(
                     controller("playback.frame_grab", ControlId::Capture, "tap"),
                     controller("global.save_replay", ControlId::Capture, "hold")),
                 Kind::SharedGesture);
    }

    void redundantRequiresIdenticalTriggerAndGesture()
    {
        QCOMPARE(BindingRelation::classify(
                     controller("global.screenshot", ControlId::Capture, "tap", {}, 1),
                     controller("global.screenshot", ControlId::Capture, "tap", {}, 2)),
                 Kind::Redundant);
    }

    void deviceSpecificRowLayersRatherThanConflicts()
    {
        // A per-controller override of the same action is the override
        // mechanism working; the resolver merges them and one survives.
        QCOMPARE(BindingRelation::classify(
                     controller("global.screenshot", ControlId::Capture, "tap", {}),
                     controller("global.screenshot", ControlId::Capture, "tap",
                                QStringLiteral("054C:0CE6"))),
                 Kind::None);
    }

    void complementaryGesturesShareOneButton()
    {
        QCOMPARE(BindingRelation::classify(
                     controller("global.screenshot", ControlId::Capture, "tap"),
                     controller("global.save_replay", ControlId::Capture, "hold")),
                 Kind::SharedGesture);
        QCOMPARE(BindingRelation::classify(
                     controller("global.screenshot", ControlId::Capture, "tap"),
                     controller("global.toggle_overlay", ControlId::Capture, "double_tap")),
                 Kind::SharedGesture);
    }

    void sameGestureOnDifferentActionsIsHardConflict()
    {
        QCOMPARE(BindingRelation::classify(
                     controller("overlay.favorite", ControlId::FaceNorth),
                     controller("overlay.menu", ControlId::FaceNorth)),
                 Kind::HardConflict);
    }

    void pressAgainstTimedGestureIsHardConflict()
    {
        // Global press vs Overlay tap on one button: press resolves on the down
        // edge and would fire alongside whatever the tap later becomes.
        QCOMPARE(BindingRelation::classify(
                     controller("global.toggle_overlay", ControlId::Menu, "press"),
                     controller("overlay.sidebar_toggle", ControlId::Menu, "tap")),
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
        QCOMPARE(BindingRelation::kindId(Kind::HardConflict), QStringLiteral("hard_conflict"));
    }
};

QTEST_GUILESS_MAIN(BindingRelationTest)
#include "tst_bindingrelation.moc"
