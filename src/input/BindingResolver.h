#pragma once

#include "input/ActionCatalog.h"

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
    };

    // The gesture a slot carries independently of what trigger sits in it.
    // Capturing into an empty slot and clearing a bound one both consult this,
    // so a slot whose meaning is "tap" stays a tap across clear and rebind.
    struct Gesture {
        QString activation = QStringLiteral("press");
        int holdMs = 0;
    };

    explicit BindingResolver(CaptureDatabase* database);

    void setDefaultHoldMs(int milliseconds);
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
                              const QString& activation,
                              ActionCatalog::Scope primaryScope,
                              ActionCatalog::Scope fallbackScope = ActionCatalog::Scope::Global) const;

    static QVector<Binding> defaultBindings(int captureHoldMs = 2000);

private:
    CaptureDatabase* m_database = nullptr;
    QVector<Binding> m_overrides;
    QHash<QString, QString> m_profileAliases;
    int m_defaultHoldMs = 2000;
};
