#include "input/CapabilityEventRouter.h"

namespace ModernInput {

QString CapabilityEventRouter::capabilityKey(const QString& logicalId,
                                             ControllerCapability capability)
{
    return logicalId + QLatin1Char(':') + QString::number(static_cast<quint32>(capability));
}

QString CapabilityEventRouter::controlKey(const QString& logicalId, const QString& controlId)
{
    return logicalId + QLatin1Char('\x1f') + controlId;
}

CapabilityRouteResult CapabilityEventRouter::route(const CapabilityControlEdge& edge)
{
    CapabilityRouteResult result;
    if (edge.logicalId.isEmpty() || edge.controlId.isEmpty())
        return result;
    const ControllerProvider preferred = m_registry
        ? m_registry->preferredProvider(edge.logicalId, edge.capability)
        : edge.provider;
    if (edge.provider != preferred)
        return result;

    const QString capKey = capabilityKey(edge.logicalId, edge.capability);
    const auto selected = m_selectedProviders.constFind(capKey);
    if (selected == m_selectedProviders.cend() || *selected != edge.provider) {
        result.providerChanged = selected != m_selectedProviders.cend();
        if (result.providerChanged) {
            for (auto it = m_held.begin(); it != m_held.end();) {
                if (it.key().startsWith(edge.logicalId + QLatin1Char('\x1f'))
                    && it->capability == edge.capability && it->provider != edge.provider) {
                    result.safeReleases.push_back(it.key().section(QLatin1Char('\x1f'), 1));
                    m_lastEdges.remove(it.key());
                    it = m_held.erase(it);
                } else {
                    ++it;
                }
            }
        }
        m_selectedProviders.insert(capKey, edge.provider);
        result.generation = ++m_generations[edge.logicalId];
    } else {
        result.generation = m_generations.value(edge.logicalId);
    }

    const QString key = controlKey(edge.logicalId, edge.controlId);
    const auto last = m_lastEdges.constFind(key);
    if (last != m_lastEdges.cend() && last->pressed == edge.pressed) {
        const quint64 delta = edge.timestamp >= last->timestamp
            ? edge.timestamp - last->timestamp : 0;
        if (last->provider == edge.provider || delta <= 35) {
            result.duplicate = true;
            return result;
        }
    }

    m_lastEdges.insert(key, {edge.pressed, edge.timestamp, edge.provider});
    if (edge.pressed)
        m_held.insert(key, {edge.provider, edge.capability});
    else
        m_held.remove(key);
    result.accepted = true;
    return result;
}

QStringList CapabilityEventRouter::disconnect(const QString& logicalId)
{
    QStringList releases;
    for (auto it = m_held.begin(); it != m_held.end();) {
        if (it.key().startsWith(logicalId + QLatin1Char('\x1f'))) {
            releases.push_back(it.key().section(QLatin1Char('\x1f'), 1));
            it = m_held.erase(it);
        } else {
            ++it;
        }
    }
    ++m_generations[logicalId];
    return releases;
}

quint64 CapabilityEventRouter::generation(const QString& logicalId) const
{
    return m_generations.value(logicalId);
}

} // namespace ModernInput
