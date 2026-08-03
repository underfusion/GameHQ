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
// treat it as a contract, not an implementation detail. Chord and tap-count
// rules were APPENDED to it, never interleaved.
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

    // A binding can be perfectly valid and still feel slow, which the user has
    // no way to explain to themselves. These are the two reasons a press waits,
    // surfaced so the editor can say so instead of leaving it a mystery.
    enum class Notice {
        None,
        // Another action on the same button needs more taps, so this one has to
        // wait out the multi-tap window before it can be sure.
        HigherTapCountDelay,
        // This button starts a combination, so its own gesture waits for the
        // chord window before acting.
        ChordStartDelay
    };

    // Frozen decision order:
    //  0) different device group, or triggers sharing no control -> None
    //  1) scopes that are never active together        -> None
    //  2) explicitly declared contextual override      -> ContextOverride
    //  3) same action + trigger + gesture + group + profile -> Redundant
    //  4) chord rules (appended): duplicate chord, press/repeat on a chord's
    //     first control, chord sharing a control with a timed gesture
    //  5) overlapping scopes, compatible gestures      -> SharedGesture
    //  6) overlapping scopes, a press gesture involved,
    //     or the same gesture on different actions     -> HardConflict
    //  7) otherwise                                    -> None
    static Kind classify(const BindingResolver::Binding& left,
                         const BindingResolver::Binding& right);

    // Why one of these two bindings waits before acting. Independent of Kind:
    // a perfectly fine SharedGesture pair can still carry a delay notice, and
    // that is exactly the case worth explaining.
    static Notice noticeFor(const BindingResolver::Binding& left,
                            const BindingResolver::Binding& right);

    // Human-readable form of a notice, for the editor. `chordWindowMs` and
    // `multiTapIntervalMs` are the live configured values, so the number the
    // user reads is the number the runtime waits.
    static QString noticeText(Notice notice, int chordWindowMs, int multiTapIntervalMs);

    // True when both bindings can dispatch at the same moment.
    //
    // Global is always live and unions with the active context. Two *different*
    // contextual scopes never dispatch together: BindingResolver::matching()
    // uses the primary scope's matches and consults the fallback only when
    // primary produced none. Playback over Desktop or Overlay is therefore a
    // priority chain, not a collision — which is why D-pad Left can ship as
    // both playback.seek_back and desktop.navigate_left.
    static bool scopesOverlap(ActionCatalog::Scope left, ActionCatalog::Scope right);

    // Tap and hold, and two taps needing a different number of taps, coexist on
    // one button because the runtime separates them in time. Anything involving
    // press does not: press fires on the down edge, before a gesture is known.
    static bool gesturesCompatible(const GestureSpec& left, const GestureSpec& right);

    // Stable lowercase identifier for QML and logs ("hard_conflict", ...).
    static QString kindId(Kind kind);
    static QString noticeId(Notice notice);
};
