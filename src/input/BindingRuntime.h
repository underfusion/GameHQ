#pragma once

#include "input/BindingRelation.h"
#include "input/BindingResolver.h"

#include <QHash>
#include <QObject>

class CaptureDatabase;

// Converts raw press/release edges into press, tap, hold, and double-tap
// actions after applying the current context and saved binding overrides.
class BindingRuntime : public QObject
{
    Q_OBJECT
public:
    explicit BindingRuntime(CaptureDatabase* database, QObject* parent = nullptr);
    ~BindingRuntime() override;

    void setDefaultHoldMs(int milliseconds);
    void reload();
    QVector<BindingResolver::Binding> effectiveBindings(
        const QString& deviceGroup, const QString& deviceProfile = {}) const;
    BindingResolver::Gesture inheritedGesture(const QString& deviceGroup,
                                              const QString& deviceProfile,
                                              const QString& actionId, int slot) const;
    void setProfileAlias(const QString& profile, const QString& legacyProfile);

    // One classified pair out of the effective table for a device group.
    struct Relation {
        BindingResolver::Binding left;
        BindingResolver::Binding right;
        BindingRelation::Kind kind = BindingRelation::Kind::None;
    };

    // Every non-None pair for this group/profile, classified once and cached
    // until the next reload(). The editor classifies its capture candidate
    // directly (the candidate is not in the table yet, so it cannot appear
    // here); tst_bindingeditor::runtimeRelationsMatchDirectClassification pins
    // both paths to the same BindingRelation::classify verdicts. Nothing on the
    // per-event path calls this — gesture dispatch works off the resolver — so
    // the O(n^2) pass costs nothing at input rates.
    const QVector<Relation>& relations(const QString& deviceGroup,
                                       const QString& deviceProfile = {}) const;

    bool press(const QString& deviceGroup, const QString& deviceProfile,
               const QString& triggerCode, ActionCatalog::Scope primaryScope,
               ActionCatalog::Scope fallbackScope = ActionCatalog::Scope::Global);
    bool release(const QString& deviceGroup, const QString& deviceProfile,
                 const QString& triggerCode);
    void cancelAll();

signals:
    void actionTriggered(const QString& actionId, const QString& triggerCode);

private:
    struct GestureState;
    QString stateKey(const QString& deviceGroup, const QString& deviceProfile,
                     const QString& triggerCode) const;
    void emitBindings(const QVector<BindingResolver::Binding>& bindings,
                      const QString& triggerCode);
    void scheduleNextHold(GestureState* state);
    void reset(GestureState* state);

    BindingResolver m_resolver;
    QHash<QString, GestureState*> m_states;
    // Cleared by reload(); repopulated on first request per group/profile.
    mutable QHash<QString, QVector<Relation>> m_relations;
};
