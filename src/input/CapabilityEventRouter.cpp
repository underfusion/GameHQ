#include "input/CapabilityEventRouter.h"

namespace ModernInput {

QString CapabilityEventRouter::controlKey(const QString& logicalId, const QString& controlId)
{
    return logicalId + QLatin1Char('\x1f') + controlId;
}

// Press-cycle ownership: the first accepted press owns the control until the
// logical release. Mirrored presses from other correlated providers join the
// cycle as participants but never transfer ownership and never emit a second
// action. A release from any participant closes the cycle exactly once; a
// release with no open cycle is explicitly ignored so a late mirror release
// can never become a spurious action. Ownership only moves when the owning
// provider detached mid-hold — then the stuck state is closed with a safe
// release and the working source starts a fresh cycle.
CapabilityRouteResult CapabilityEventRouter::route(const CapabilityControlEdge& edge)
{
    CapabilityRouteResult result;
    if (edge.logicalId.isEmpty() || edge.controlId.isEmpty())
        return result;
    const QString key = controlKey(edge.logicalId, edge.controlId);
    const auto held = m_held.find(key);
    result.generation = m_generations.value(edge.logicalId);

    if (edge.pressed) {
        if (held != m_held.end()) {
            if (edge.provider == held->provider) {
                // Repeated press report from the owner while its cycle is
                // still open — a report echo, never a second action.
                result.duplicate = true;
                return result;
            }
            const auto* logical = m_registry
                ? m_registry->controller(edge.logicalId) : nullptr;
            if (logical && logical->hasProvider(held->provider)) {
                // Mirrored press from a correlated provider: it joins the
                // open cycle so its release may close it, but ownership never
                // moves — otherwise the owner's release would be rejected and
                // the control would stay held forever (0.7.4 Share/PS bug).
                held->downProviders.insert(edge.provider);
                result.duplicate = true;
                return result;
            }
            // A failed/detached owner cannot retain a held control. Release
            // its state once, then let the working source open a fresh cycle.
            result.safeReleases.push_back(edge.controlId);
            result.providerChanged = true;
            m_held.erase(held);
            result.generation = ++m_generations[edge.logicalId];
        }
        HeldEdge cycle{edge.provider, edge.capability, {edge.provider}};
        m_held.insert(key, cycle);
        result.accepted = true;
        return result;
    }

    if (held == m_held.end()) {
        // No open cycle: a late mirror release after the cycle already closed
        // (or a stray release) must never become an accepted release.
        result.duplicate = true;
        return result;
    }
    if (!held->downProviders.contains(edge.provider)) {
        const auto* logical = m_registry
            ? m_registry->controller(edge.logicalId) : nullptr;
        if (!logical || !logical->hasProvider(held->provider)) {
            // The owner died mid-hold and only a non-participant source saw
            // the physical release: close the stuck cycle safely without
            // treating the foreign release as an action of its own.
            result.safeReleases.push_back(edge.controlId);
            result.providerChanged = true;
            m_held.erase(held);
            result.generation = ++m_generations[edge.logicalId];
        }
        result.duplicate = true;
        return result;
    }
    // Any participant of the open cycle closes it — exactly once, because the
    // cycle state is discarded here and rebuilt only by the next fresh press.
    m_held.erase(held);
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
