#pragma once

#include "input/ActionCatalog.h"
#include "input/BindingPattern.h"

#include <QHash>
#include <QString>
#include <QVector>

class CaptureDatabase;

// Merges code-owned defaults with the sparse user overrides stored in SQLite.
// Runtime input and the binding editor both consume this same effective view.
class BindingResolver
{
public:
    struct Binding {
        QString deviceGroup;   // keyboard | controller | mouse
        QString deviceProfile; // empty = all devices; otherwise a fingerprint
        QString actionId;
        int slot = 1;
        QString triggerCode;
        QString activation = QStringLiteral("press");
        int holdMs = 0;
        bool unbound = false;
        // Taps a "tap" activation needs, 1-3. Meaningless for press and hold,
        // where the model pins it to 1.
        int tapCount = 1;

        // The gesture as the pattern model sees it. Rows that reach here have
        // already passed validation in reload(), so this never fails; a
        // hand-built Binding with a nonsense gesture degrades to plain press
        // rather than throwing at a call site that only wanted to compare two
        // bindings.
        // Inline on purpose: the conflict policy is a pure-logic unit and must
        // not have to link the resolver (and through it the database) just to
        // ask a binding what gesture it carries.
        GestureSpec gesture() const
        {
            const auto parsed = GestureSpec::parse(activation, tapCount, holdMs);
            return parsed.ok ? parsed.gesture : GestureSpec::press();
        }
        TriggerSpec trigger() const
        {
            const auto parsed = TriggerSpec::parse(triggerCode);
            return parsed.ok ? parsed.trigger : TriggerSpec::single(triggerCode);
        }
    };

    // The gesture a slot carries independently of what trigger sits in it.
    // Capturing into an empty slot and clearing a bound one both consult this,
    // so a slot whose meaning is "tap" stays a tap across clear and rebind.
    struct Gesture {
        QString activation = QStringLiteral("press");
        int holdMs = 0;
        int tapCount = 1;

        GestureSpec spec() const
        {
            const auto parsed = GestureSpec::parse(activation, tapCount, holdMs);
            return parsed.ok ? parsed.gesture : GestureSpec::press();
        }
    };

    explicit BindingResolver(CaptureDatabase* database);

    void setDefaultHoldMs(int milliseconds);
    // The duration a Hold binding with holdMs == 0 means. Built-in hold
    // defaults store 0 on purpose, so the configured value is the single
    // source of truth instead of being baked into every default row.
    int defaultHoldMs() const { return m_defaultHoldMs; }
    void reload();

    Gesture inheritedGesture(const QString& deviceGroup, const QString& deviceProfile,
                             const QString& actionId, int slot) const;

    // Legacy-profile aliasing: rows saved under `legacyProfile` (an
    // "xinput.slotN" fingerprint from before stable identity existed) keep
    // applying while `profile` is active, at lower precedence than rows saved
    // for `profile` itself. Existing overrides survive the upgrade unchanged;
    // they are never silently rewritten or broadened — promotion to the
    // stable identity is the user's explicit copy action in Settings.
    void setProfileAlias(const QString& profile, const QString& legacyProfile);

    QVector<Binding> effectiveBindings(const QString& deviceGroup,
                                       const QString& deviceProfile = {}) const;
    QVector<Binding> matching(const QString& deviceGroup,
                              const QString& deviceProfile,
                              const QString& triggerCode,
                              const GestureSpec& gesture,
                              ActionCatalog::Scope primaryScope,
                              ActionCatalog::Scope fallbackScope = ActionCatalog::Scope::Global) const;

    static QVector<Binding> defaultBindings();

private:
    CaptureDatabase* m_database = nullptr;
    QVector<Binding> m_overrides;
    QHash<QString, QString> m_profileAliases;
    int m_defaultHoldMs = 2000;
};
