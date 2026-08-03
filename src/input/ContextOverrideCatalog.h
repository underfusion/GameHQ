#pragma once

#include "input/BindingPattern.h"

#include <QString>
#include <QVector>

// Hand-declared list of contextual actions that *replace* a Global action
// instead of firing alongside it.
//
// The binding resolver unions Global bindings with the ones from the active
// contextual scope, so a trigger bound in both places normally fires both
// actions. That is usually what we want (Guide stays a global overlay toggle
// no matter what is focused). It is wrong only for the few pairs where the
// contextual action is a deliberate substitution of the global one.
//
// This catalog stays explicit on purpose. A generic "any contextual action
// shadows Global" rule would silently swallow user-created overlaps, which the
// binding editor must instead surface and classify. Every entry here is a
// product decision, not an inferred one.
class ContextOverrideCatalog
{
public:
    struct Override {
        QString overridingActionId; // contextual action that wins
        QString shadowedActionId;   // Global action it replaces
        GestureSpec gesture;        // gesture the substitution applies to
    };

    static const QVector<Override>& all();

    // True when shadowedActionId must be suppressed because overridingActionId
    // resolved for the same trigger and gesture in the active scope. The
    // gesture is matched by kind and tap count — a substitution declared for a
    // single tap must not swallow the global action on a double tap.
    static bool shadows(const QString& overridingActionId,
                        const QString& shadowedActionId,
                        const GestureSpec& gesture);
};
