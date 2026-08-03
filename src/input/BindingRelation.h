#pragma once

#include "input/ActionCatalog.h"
#include "input/BindingResolver.h"

#include <QString>

// Single source of truth for how two bindings relate to each other.
//
// The binding editor needs this to warn the user, and the runtime needs the
// same answer so a warning can never disagree with what the buttons actually
// do. Before this policy existed the editor asked scopesConflict(), which was
// literally `left == right` — it treated every same-scope overlap as fatal and
// every cross-scope overlap as harmless, so it flagged legitimate tap/hold
// sharing and stayed silent on real Global-vs-Overlay collisions.
//
// The decision order below is frozen: classify() walks it top to bottom and
// returns on the first match. Reordering it changes user-visible verdicts, so
// treat it as a contract, not an implementation detail.
class BindingRelation
{
public:
    enum class Kind {
        None,            // the two bindings never compete
        ContextOverride, // declared substitution: the contextual action replaces the Global one
        Redundant,       // same action, same trigger, same gesture, same device and profile
        SharedGesture,   // same trigger, complementary gestures, overlapping scopes
        HardConflict     // same trigger, colliding gestures, different actions
    };

    // Frozen decision order:
    //  0) different device group or different trigger  -> None (precondition)
    //  1) scopes that are never active together        -> None
    //  2) explicitly declared contextual override      -> ContextOverride
    //  3) same action + trigger + activation + group + profile -> Redundant
    //  4) overlapping scopes, compatible gestures      -> SharedGesture
    //  5) overlapping scopes, a press gesture involved,
    //     or the same gesture on different actions     -> HardConflict
    //  6) otherwise                                    -> None
    static Kind classify(const BindingResolver::Binding& left,
                         const BindingResolver::Binding& right);

    // True when both bindings can dispatch at the same moment.
    //
    // Global is always live and unions with the active context. Two *different*
    // contextual scopes never dispatch together: BindingResolver::matching()
    // uses the primary scope's matches and consults the fallback only when
    // primary produced none. Playback over Desktop or Overlay is therefore a
    // priority chain, not a collision — which is why D-pad Left can ship as
    // both playback.seek_back and desktop.navigate_left.
    static bool scopesOverlap(ActionCatalog::Scope left, ActionCatalog::Scope right);

    // tap/hold, tap/double_tap and hold/double_tap coexist on one button
    // because the runtime separates them in time. Anything involving "press"
    // does not: press fires on the down edge, before a gesture is known.
    static bool gesturesCompatible(const QString& left, const QString& right);

    // Stable lowercase identifier for QML and logs ("hard_conflict", ...).
    static QString kindId(Kind kind);
};
