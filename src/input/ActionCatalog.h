#pragma once
#include <QString>
#include <QVector>

// Stable catalog of every bindable application behavior.
// This is the single source of truth for action identity, scope, and label —
// the binding resolver and runtime dispatch both read from it
// instead of hard-coding button-to-behavior switches.
class ActionCatalog
{
public:
    // Context an action fires in. An action only dispatches while its scope
    // is the active one, so the same physical trigger can mean different
    // things in the overlay vs. the desktop gallery vs. clip playback.
    enum class Scope {
        Global,     // active regardless of what else is showing
        Desktop,    // desktop gallery has real Win32 foreground focus
        Overlay,    // in-game overlay is visible
        Playback    // a clip is focused/playing in the overlay or gallery lightbox
    };

    struct Action {
        QString id;             // stable identifier, e.g. "global.screenshot"
        Scope scope;
        QString label;          // short UI label, e.g. "Screenshot"
        QString description;    // one-line explanation for the binding editor
        bool bindable = true;   // false = fixed emergency route, never remappable
    };

    // Full catalog, in a stable declaration order (used for default UI grouping).
    static const QVector<Action>& all();

    // Returns nullptr if id is unknown.
    static const Action* find(const QString& id);
};
