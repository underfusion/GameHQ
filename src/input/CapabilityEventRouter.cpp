#include "input/CapabilityEventRouter.h"

namespace ModernInput {

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
        ? m_registry->preferredProvider(edge.logicalId, edge.capability, edge.controlId)
        : edge.provider;
    const QString key = controlKey(edge.logicalId, edge.controlId);
    const auto selected = m_selectedProviders.constFind(key);
    if (selected == m_selectedProviders.cend() || *selected != edge.provider) {
        result.providerChanged = selected != m_selectedProviders.cend();
        if (result.providerChanged) {
            auto held = m_held.constFind(key);
            if (edge.pressed && held != m_held.cend()) {
                const auto* logical = m_registry
                    ? m_registry->controller(edge.logicalId) : nullptr;
                const bool previousProviderStillLive = logical
                    && logical->hasProvider(*selected);
                if (!previousProviderStillLive) {
                    // A failed/detached owner cannot retain a held control.
                    // Release its state once, then allow the working fallback
                    // source to establish a fresh down edge below.
                    result.safeReleases.push_back(edge.controlId);
                    m_held.remove(key);
                    m_lastEdges.remove(key);
                    held = m_held.cend();
                }
                // A mirrored provider cannot create a second down edge while
                // the canonical control is already held. A newly preferred
                // provider may take ownership silently so its release wins.
                if (held != m_held.cend() && edge.provider == preferred) {
                    m_selectedProviders.insert(key, edge.provider);
                    m_held[key].provider = edge.provider;
                    result.generation = ++m_generations[edge.logicalId];
                }
                if (held != m_held.cend()) {
                    result.duplicate = true;
                    return result;
                }
            }
            if (edge.provider != preferred)
                return result;
            if (!edge.pressed && held != m_held.cend()) {
                result.safeReleases.push_back(edge.controlId);
                m_held.remove(key);
                m_lastEdges.insert(key, {false, edge.timestamp, edge.provider});
                m_selectedProviders.insert(key, edge.provider);
                result.generation = ++m_generations[edge.logicalId];
                result.duplicate = true;
                return result;
            }
        }
        // No provider has proved it can deliver this exact control yet. Accept
        // the first working source even when a higher-ranked registered
        // provider is silent; a later preferred mirror transfers ownership.
        m_selectedProviders.insert(key, edge.provider);
        result.generation = ++m_generations[edge.logicalId];
    } else {
        result.generation = m_generations.value(edge.logicalId);
    }

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

QStringList CapabilityEventRouter::resetLogicalController(const QString& logicalId)
{
    QStringList releases;
    if (logicalId.isEmpty())
        return releases;
    const QString controlPrefix = logicalId + QLatin1Char('\x1f');
    for (auto it = m_held.begin(); it != m_held.end();) {
        if (it.key().startsWith(controlPrefix)) {
            releases.push_back(it.key().section(QLatin1Char('\x1f'), 1));
            it = m_held.erase(it);
        } else {
            ++it;
        }
    }
    for (auto it = m_lastEdges.begin(); it != m_lastEdges.end();) {
        if (it.key().startsWith(controlPrefix))
            it = m_lastEdges.erase(it);
        else
            ++it;
    }
    for (auto it = m_selectedProviders.begin(); it != m_selectedProviders.end();) {
        if (it.key().startsWith(controlPrefix))
            it = m_selectedProviders.erase(it);
        else
            ++it;
    }
    ++m_generations[logicalId];
    return releases;
}

QStringList CapabilityEventRouter::disconnect(const QString& logicalId)
{
    return resetLogicalController(logicalId);
}

quint64 CapabilityEventRouter::generation(const QString& logicalId) const
{
    return m_generations.value(logicalId);
}

} // namespace ModernInput
