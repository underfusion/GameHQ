#include "input/PhysicalControllerRegistry.h"

#include <QCryptographicHash>

#include <algorithm>

namespace ModernInput {

ControllerCapabilities LogicalController::capabilities() const
{
    ControllerCapabilities result;
    for (const auto& attachment : providers)
        result |= attachment.capabilities;
    return result;
}

bool LogicalController::hasProvider(ControllerProvider provider) const
{
    return std::any_of(providers.cbegin(), providers.cend(),
                       [provider](const auto& item) { return item.provider == provider; });
}

QString PhysicalControllerRegistry::attachmentKey(ControllerProvider provider,
                                                   const QString& providerDeviceId)
{
    return QStringLiteral("%1:%2").arg(static_cast<int>(provider)).arg(providerDeviceId);
}

QString PhysicalControllerRegistry::createLogicalId(const ProviderObservation& observation,
                                                     quint64 generation)
{
    // A strong identity hashes deterministically from the identity namespace
    // alone: the same physical device must produce the same logical ID
    // regardless of provider arrival order, detection order, or session —
    // this ID is persisted in binding profiles and controller_layouts. Only
    // weak identities (nothing stable to hash) stay session-local through
    // the generation counter.
    const bool strongIdentity = !observation.appLocalDeviceId.isEmpty()
        || !observation.containerId.isEmpty();
    const QString strongest = !observation.appLocalDeviceId.isEmpty()
        ? observation.appLocalDeviceId
        : (!observation.containerId.isEmpty() ? observation.containerId
                                              : observation.providerDeviceId);
    const QByteArray material = strongIdentity
        ? QStringLiteral("strong|%1").arg(strongest).toUtf8()
        : QStringLiteral("weak|%1|%2|%3")
              .arg(static_cast<int>(observation.provider))
              .arg(strongest).arg(generation).toUtf8();
    return QStringLiteral("controller-%1").arg(QString::fromLatin1(
        QCryptographicHash::hash(material, QCryptographicHash::Sha256).toHex().left(16)));
}

QString PhysicalControllerRegistry::findStrongMatch(const ProviderObservation& observation) const
{
    for (auto it = m_controllers.cbegin(); it != m_controllers.cend(); ++it) {
        const auto& current = it.value();
        if (!observation.appLocalDeviceId.isEmpty()
            && observation.appLocalDeviceId == current.appLocalDeviceId)
            return it.key();
        // A shared container may hold several endpoints (hub, receiver): a
        // container match must never merge two conflicting strong device IDs.
        if (!observation.containerId.isEmpty()
            && observation.containerId == current.containerId) {
            if (!observation.appLocalDeviceId.isEmpty()
                && !current.appLocalDeviceId.isEmpty()
                && observation.appLocalDeviceId != current.appLocalDeviceId)
                continue;
            return it.key();
        }
    }
    return {};
}

QString PhysicalControllerRegistry::findCorrelatedMatch(const ProviderObservation& observation) const
{
    if (observation.topologyRoot.isEmpty())
        return {};

    QString match;
    for (auto it = m_controllers.cbegin(); it != m_controllers.cend(); ++it) {
        if (it->topologyRoot != observation.topologyRoot)
            continue;
        // One provider observes each physical device separately: a controller
        // that already carries a live attachment from this provider cannot be
        // the same physical pad — it is its identical twin. Without this, a
        // second same-model pad would silently merge into the first.
        if (it->hasProvider(observation.provider))
            continue;
        if (!observation.appLocalDeviceId.isEmpty() && !it->appLocalDeviceId.isEmpty()
            && observation.appLocalDeviceId != it->appLocalDeviceId)
            continue;
        if (!observation.containerId.isEmpty() && !it->containerId.isEmpty()
            && observation.containerId != it->containerId)
            continue;
        // Correlation is accepted only when the root identifies exactly one
        // logical controller. Ambiguity deliberately creates a new identity.
        if (!match.isEmpty())
            return {};
        match = it.key();
    }
    return match;
}

QString PhysicalControllerRegistry::observe(const ProviderObservation& observation)
{
    const QString key = attachmentKey(observation.provider, observation.providerDeviceId);
    if (const auto existing = m_attachmentToLogical.constFind(key);
        existing != m_attachmentToLogical.cend()) {
        // Re-observation of a live attachment (e.g. CapabilityChanged) must
        // refresh its capabilities and fill in missing metadata, not no-op.
        if (const auto it = m_controllers.find(*existing); it != m_controllers.end()) {
            for (auto& attachment : it->providers) {
                if (attachment.provider == observation.provider
                    && attachment.providerDeviceId == observation.providerDeviceId)
                    attachment.capabilities = observation.capabilities;
            }
            if (it->displayName.isEmpty())
                it->displayName = observation.displayName;
        }
        return *existing;
    }

    QString logicalId = findStrongMatch(observation);
    IdentityConfidence confidence = IdentityConfidence::Strong;
    if (logicalId.isEmpty()) {
        logicalId = findCorrelatedMatch(observation);
        confidence = IdentityConfidence::Correlated;
    }
    // A strong observation merging into a controller that was created from a
    // weak identity (a legacy provider observed first) upgrades the logical
    // ID to the deterministic strong hash. Persisted per-controller state
    // (binding profiles, extra-button layouts) is keyed on that ID, so the
    // upgrade is what reconnects a device to its saved profile regardless of
    // which provider observed it first this session.
    if (!logicalId.isEmpty()
        && (!observation.appLocalDeviceId.isEmpty() || !observation.containerId.isEmpty())) {
        const auto existing = m_controllers.constFind(logicalId);
        if (existing != m_controllers.cend()
            && existing->appLocalDeviceId.isEmpty() && existing->containerId.isEmpty()) {
            const QString upgradedId = createLogicalId(observation, 0);
            if (upgradedId != logicalId && !m_controllers.contains(upgradedId)) {
                LogicalController moved = m_controllers.take(logicalId);
                moved.logicalId = upgradedId;
                m_controllers.insert(upgradedId, moved);
                for (auto it = m_attachmentToLogical.begin();
                     it != m_attachmentToLogical.end(); ++it) {
                    if (it.value() == logicalId)
                        it.value() = upgradedId;
                }
                logicalId = upgradedId;
            }
        }
    }
    if (logicalId.isEmpty()) {
        logicalId = createLogicalId(observation, ++m_generation);
        confidence = (!observation.appLocalDeviceId.isEmpty() || !observation.containerId.isEmpty())
            ? IdentityConfidence::Strong
            : (!observation.topologyRoot.isEmpty() ? IdentityConfidence::Correlated
                                                   : IdentityConfidence::Weak);
        LogicalController controller;
        controller.logicalId = logicalId;
        controller.displayName = observation.displayName;
        controller.confidence = confidence;
        controller.appLocalDeviceId = observation.appLocalDeviceId;
        controller.containerId = observation.containerId;
        controller.topologyRoot = observation.topologyRoot;
        controller.vendorId = observation.vendorId;
        controller.productId = observation.productId;
        m_controllers.insert(logicalId, controller);
    }

    auto& controller = m_controllers[logicalId];
    if (controller.displayName.isEmpty())
        controller.displayName = observation.displayName;
    if (controller.appLocalDeviceId.isEmpty())
        controller.appLocalDeviceId = observation.appLocalDeviceId;
    if (controller.containerId.isEmpty())
        controller.containerId = observation.containerId;
    if (controller.topologyRoot.isEmpty())
        controller.topologyRoot = observation.topologyRoot;
    controller.confidence = std::max(controller.confidence, confidence);
    controller.providers.push_back({observation.provider, observation.providerDeviceId,
                                    observation.capabilities});
    m_attachmentToLogical.insert(key, logicalId);
    return logicalId;
}

bool PhysicalControllerRegistry::removeProvider(ControllerProvider provider,
                                                const QString& providerDeviceId)
{
    const QString key = attachmentKey(provider, providerDeviceId);
    const QString logicalId = m_attachmentToLogical.take(key);
    if (logicalId.isEmpty())
        return false;
    auto it = m_controllers.find(logicalId);
    if (it == m_controllers.end())
        return false;
    auto& providers = it->providers;
    providers.erase(std::remove_if(providers.begin(), providers.end(),
                                   [&](const auto& item) {
                                       return item.provider == provider
                                           && item.providerDeviceId == providerDeviceId;
                                   }), providers.end());
    // Retain the logical identity while disconnected. A later observation
    // carrying the same strong ID restores this controller's profile; weak
    // observations still create a separate logical controller.
    return true;
}

const LogicalController* PhysicalControllerRegistry::controller(const QString& logicalId) const
{
    const auto it = m_controllers.constFind(logicalId);
    return it == m_controllers.cend() ? nullptr : &it.value();
}

QVector<LogicalController> PhysicalControllerRegistry::controllers() const
{
    return m_controllers.values();
}

QString PhysicalControllerRegistry::logicalIdFor(ControllerProvider provider,
                                                 const QString& providerDeviceId) const
{
    return m_attachmentToLogical.value(attachmentKey(provider, providerDeviceId));
}

ControllerProvider PhysicalControllerRegistry::preferredProvider(
    const QString& logicalId, ControllerCapability capability) const
{
    const auto* logical = controller(logicalId);
    if (!logical)
        return ControllerProvider::WinMM;

    const auto supports = [&](ControllerProvider provider) {
        return std::any_of(logical->providers.cbegin(), logical->providers.cend(),
                           [&](const auto& item) {
                               return item.provider == provider
                                   && item.capabilities.testFlag(capability);
                           });
    };
    const QVector<ControllerProvider> order = capability == ControllerCapability::StandardControls
        ? QVector<ControllerProvider>{ControllerProvider::SonyRaw, ControllerProvider::GameInput,
                                      ControllerProvider::XInput, ControllerProvider::WinMM}
        : QVector<ControllerProvider>{ControllerProvider::GameInput, ControllerProvider::SonyRaw,
                                      ControllerProvider::RawHid, ControllerProvider::XInput,
                                      ControllerProvider::WinMM};
    for (ControllerProvider provider : order) {
        if (supports(provider))
            return provider;
    }
    return ControllerProvider::WinMM;
}

} // namespace ModernInput
