#pragma once

#include "input/PhysicalControllerRegistry.h"

#include <QHash>
#include <QStringList>

namespace ModernInput {

struct CapabilityControlEdge {
    QString logicalId;
    ControllerProvider provider = ControllerProvider::WinMM;
    ControllerCapability capability = ControllerCapability::StandardControls;
    QString controlId;
    bool pressed = false;
    quint64 timestamp = 0;
};

struct CapabilityRouteResult {
    bool accepted = false;
    bool duplicate = false;
    bool providerChanged = false;
    quint64 generation = 0;
    QStringList safeReleases;
};

class CapabilityEventRouter
{
public:
    explicit CapabilityEventRouter(const PhysicalControllerRegistry* registry = nullptr)
        : m_registry(registry) {}

    CapabilityRouteResult route(const CapabilityControlEdge& edge);
    QStringList disconnect(const QString& logicalId);
    quint64 generation(const QString& logicalId) const;

private:
    static QString capabilityKey(const QString& logicalId, ControllerCapability capability);
    static QString controlKey(const QString& logicalId, const QString& controlId);

    struct LastEdge { bool pressed = false; quint64 timestamp = 0; ControllerProvider provider; };
    struct HeldEdge { ControllerProvider provider; ControllerCapability capability; };

    const PhysicalControllerRegistry* m_registry = nullptr;
    QHash<QString, ControllerProvider> m_selectedProviders;
    QHash<QString, LastEdge> m_lastEdges;
    QHash<QString, HeldEdge> m_held;
    QHash<QString, quint64> m_generations;
};

} // namespace ModernInput
