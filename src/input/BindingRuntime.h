#pragma once

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
};
