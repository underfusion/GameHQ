#include "input/BindingRelation.h"

#include "input/ContextOverrideCatalog.h"

namespace {

bool isPressGesture(const QString& activation)
{
    // Empty is treated as "press": that is the resolver's default for rows
    // written before activations existed.
    return activation.isEmpty() || activation == QLatin1String("press");
}

} // namespace

bool BindingRelation::scopesOverlap(ActionCatalog::Scope left, ActionCatalog::Scope right)
{
    using Scope = ActionCatalog::Scope;
    // Global is always live, so it unions with whatever context is active.
    if (left == Scope::Global || right == Scope::Global)
        return true;
    // Two different contextual scopes never dispatch together. The resolver
    // takes the primary scope's matches and falls back to the secondary only
    // when primary produced none, so Playback-over-Desktop (gallery lightbox)
    // and Playback-over-Overlay are priority chains, not collisions: binding
    // D-pad Left to both playback.seek_back and desktop.navigate_left is the
    // shipped default and is correct.
    return left == right;
}

bool BindingRelation::gesturesCompatible(const QString& left, const QString& right)
{
    if (left == right)
        return false;
    if (isPressGesture(left) || isPressGesture(right))
        return false;
    // Everything left is a distinct pair drawn from tap / hold / double_tap.
    return true;
}

BindingRelation::Kind BindingRelation::classify(const BindingResolver::Binding& left,
                                                const BindingResolver::Binding& right)
{
    // 0) Preconditions. A keyboard chord and a controller button never compete,
    //    and neither do two different triggers — the same action reached from
    //    both Ctrl+Shift+S and F12 is a deliberate second slot, not a fault.
    if (left.deviceGroup != right.deviceGroup)
        return Kind::None;
    if (left.triggerCode != right.triggerCode || left.triggerCode.isEmpty())
        return Kind::None;
    if (left.unbound || right.unbound)
        return Kind::None;

    const ActionCatalog::Action* leftAction = ActionCatalog::find(left.actionId);
    const ActionCatalog::Action* rightAction = ActionCatalog::find(right.actionId);
    if (!leftAction || !rightAction)
        return Kind::None;

    // 1) Scopes that are never active together cannot collide.
    if (!scopesOverlap(leftAction->scope, rightAction->scope))
        return Kind::None;

    // 2) A declared substitution is expected behavior, not a conflict. Both
    //    directions are checked because argument order is the caller's choice.
    if (ContextOverrideCatalog::shadows(left.actionId, right.actionId, left.activation)
        || ContextOverrideCatalog::shadows(right.actionId, left.actionId, right.activation)) {
        return Kind::ContextOverride;
    }

    if (left.actionId == right.actionId) {
        // 3) Truly duplicated work: the same action reached twice the same way.
        if (left.activation == right.activation && left.deviceProfile == right.deviceProfile)
            return Kind::Redundant;
        // A device-specific row layered over a group-wide row for the same
        // action is the override mechanism working as designed; the resolver
        // merges them and only one survives.
        if (left.deviceProfile != right.deviceProfile)
            return Kind::None;
    }

    // 4) Complementary gestures share one button on purpose.
    if (gesturesCompatible(left.activation, right.activation))
        return Kind::SharedGesture;

    // 5) Either the same gesture drives two different actions, or a press
    //    binding sits on a button that also carries a timed gesture. Press
    //    fires on the down edge, so it always beats and doubles up with the
    //    gesture that would have resolved later.
    return Kind::HardConflict;
}

QString BindingRelation::kindId(Kind kind)
{
    switch (kind) {
    case Kind::None: return QStringLiteral("none");
    case Kind::ContextOverride: return QStringLiteral("context_override");
    case Kind::Redundant: return QStringLiteral("redundant");
    case Kind::SharedGesture: return QStringLiteral("shared_gesture");
    case Kind::HardConflict: return QStringLiteral("hard_conflict");
    }
    return QStringLiteral("none");
}
