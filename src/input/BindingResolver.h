#pragma once

#include "input/ActionCatalog.h"

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

    explicit BindingResolver(CaptureDatabase* database);

    void setDefaultHoldMs(int milliseconds);
    void reload();

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
    int m_defaultHoldMs = 2000;
};
