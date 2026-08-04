#include "gameinput/GameInputRouter.h"

#include "gameinput/GameInputWrapper.h"
#include "gameinput/IGameInputApi.h"
#include "input/ControlId.h"
#include "input/InputDiagnostics.h"
#include "input/ExtraButtonCatalog.h"
#include "input/CapabilityEventRouter.h"
#include "input/ProviderIntegration.h"

#include <algorithm>

namespace ModernInput {

namespace {
// Mirror of GameInputSystemButtons (GameInput.h); redeclared so this
// translation unit stays buildable without the vendored SDK header.
constexpr quint32 kSystemButtonGuide = 0x00000001u;
constexpr quint32 kSystemButtonShare = 0x00000002u;

QString providerLabel(ControllerProvider provider)
{
    switch (provider) {
    case ControllerProvider::SonyRaw: return QStringLiteral("Sony Raw Input");
    case ControllerProvider::GameInput: return QStringLiteral("GameInput");
    case ControllerProvider::XInput: return QStringLiteral("XInput");
    case ControllerProvider::WinMM: return QStringLiteral("WinMM");
    case ControllerProvider::RawHid: return QStringLiteral("Raw HID");
    }
    return QStringLiteral("Unknown");
}
}

GameInputRouter::GameInputRouter(std::unique_ptr<IGameInputApi> api, SupportMode mode,
                                 CaptureDatabase* database,
                                 QObject* parent)
    : QObject(parent)
    , m_wrapper(std::make_unique<GameInputWrapper>(std::move(api)))
    , m_extraButtons(std::make_unique<ExtraButtonCatalog>(database))
    , m_mode(mode)
{
    m_ownedIntegration = std::make_unique<ProviderIntegration>();
    m_integration = m_ownedIntegration.get();
    connect(m_wrapper.get(), &GameInputWrapper::eventsReady,
            this, &GameInputRouter::handleBatch);
}

void GameInputRouter::setProviderIntegration(ProviderIntegration* integration)
{
    if (!integration || integration == m_integration)
        return;
    Q_ASSERT(m_deviceLogicalIds.isEmpty());   // must be installed before start()
    m_integration = integration;
    m_ownedIntegration.reset();
}

PhysicalControllerRegistry& GameInputRouter::registryRef()
{
    return m_integration->registry();
}

CapabilityEventRouter& GameInputRouter::routerRef()
{
    return m_integration->capabilityRouter();
}

const PhysicalControllerRegistry& GameInputRouter::registry() const
{
    return m_integration->registry();
}

GameInputRouter::~GameInputRouter()
{
    shutdown();
}

bool GameInputRouter::start()
{
    if (m_mode == SupportMode::Off) {
        m_runtimeStatus = QStringLiteral("Off");
        emit statusChanged();
        return true;
    }
    if (m_failedForSession || m_active)
        return m_active;
    QString error;
    if (!m_wrapper->start(error)) {
        m_runtimeStatus = error.isEmpty() ? QStringLiteral("Unavailable") : error;
        m_failedForSession = true;
        emit statusChanged();
        emit sessionFallback(m_runtimeStatus);
        return false;
    }
    m_active = true;
    m_runtimeStatus = m_wrapper->runtimeDescription();
    InputDiagnostics::instance().noteBackendSwitch(QStringLiteral("GameInput shadow"),
                                                   QStringLiteral("Stage A active"));
    emit statusChanged();
    return true;
}

void GameInputRouter::shutdown()
{
    if (!m_active && m_heldSystemControls.isEmpty())
        return;
    releaseHeldControls();
    m_wrapper->shutdown();
    m_active = false;
    detachFromRegistry();
    if (m_mode == SupportMode::Off)
        m_runtimeStatus = QStringLiteral("Off");
    emit statusChanged();
}

// GameInput stopped delivering (Off, shutdown, or session failure): its
// attachments must leave the shared registry, otherwise preferredProvider()
// keeps electing GameInput for Share/Guide and the shared router would
// reject the legacy edges that are now the only live path.
void GameInputRouter::detachFromRegistry()
{
    for (auto it = m_deviceLogicalIds.cbegin(); it != m_deviceLogicalIds.cend(); ++it) {
        routerRef().disconnect(it.value());
        registryRef().removeProvider(ControllerProvider::GameInput, it.key());
    }
    m_deviceLogicalIds.clear();
    m_deviceExtraStates.clear();
    m_deviceExtraControls.clear();
    m_deviceStandardButtons.clear();
}

void GameInputRouter::setMode(SupportMode mode)
{
    if (m_mode == mode)
        return;
    m_mode = mode;
    if (mode == SupportMode::Off)
        shutdown();
    else
        start();
}

QString GameInputRouter::observeDevice(const GameInputDeviceDescriptor& device)
{
    if (device.deviceId.isEmpty())
        return {};
    ProviderObservation observation;
    observation.provider = ControllerProvider::GameInput;
    observation.providerDeviceId = device.deviceId;
    observation.appLocalDeviceId = device.deviceId;
    observation.containerId = device.containerId;
    observation.displayName = device.displayName;
    observation.vendorId = device.vendorId;
    observation.productId = device.productId;
    // The lowercase "vvvv:pppp" fingerprint is the only identity legacy
    // providers can also produce, so it is the topology correlation root
    // that lets Sony Raw/XInput/WinMM attachments merge onto this logical
    // controller (t25 cross-provider dedup). The registry's unique-match
    // rule keeps two identical models apart.
    if (device.vendorId != 0 || device.productId != 0) {
        observation.topologyRoot = QStringLiteral("%1:%2")
            .arg(device.vendorId, 4, 16, QLatin1Char('0'))
            .arg(device.productId, 4, 16, QLatin1Char('0'));
    }
    observation.capabilities = ControllerCapability::StandardControls;
    // Each system button is granted individually: a Guide-only pad must not
    // be reported (or routed) as Share-capable, and vice versa.
    if (device.supportedSystemButtons & kSystemButtonShare)
        observation.capabilities |= ControllerCapability::SystemShare;
    if (device.supportedSystemButtons & kSystemButtonGuide)
        observation.capabilities |= ControllerCapability::Guide;
    if (device.extraButtonCount > 0)
        observation.capabilities |= ControllerCapability::ExtraControls;
    const QString logicalId = registryRef().observe(observation);
    m_deviceLogicalIds.insert(device.deviceId, logicalId);
    m_deviceNames.insert(device.deviceId, device.displayName);
    const bool firstObservation = !m_descriptors.contains(logicalId);
    m_descriptors.insert(logicalId, device);
    InputDiagnostics::instance().noteDevice(
        QStringLiteral("%1:%2").arg(device.vendorId, 4, 16, QLatin1Char('0'))
                                  .arg(device.productId, 4, 16, QLatin1Char('0')),
        device.displayName, QStringLiteral("GameInput shadow; system buttons routed"));
    if (firstObservation)
        emit statusChanged();
    return logicalId;
}

void GameInputRouter::handleBatch(const GameInputEventBatch& batch)
{
    if (!m_active)
        return;
    if (batch.forceResync) {
        failSession(QStringLiteral("GameInput queue overflow required state resynchronization"));
        return;
    }

    for (const auto& event : batch.events) {
        if (event.kind == GameInputEventKind::DeviceAdded
            || event.kind == GameInputEventKind::Wake)
            m_removedDevices.remove(event.deviceId);
        // A reading or button event trailing its device's removal (late
        // callback or queue tail) must not resurrect the device; only a new
        // DeviceAdded/Wake may clear the tombstone.
        if (m_removedDevices.contains(event.deviceId)
            && (event.kind == GameInputEventKind::Reading
                || event.kind == GameInputEventKind::SystemButtonPressed
                || event.kind == GameInputEventKind::SystemButtonReleased))
            continue;
        QString logicalId = m_deviceLogicalIds.value(event.deviceId);
        if (!event.device.deviceId.isEmpty())
            logicalId = observeDevice(event.device);
        switch (event.kind) {
        case GameInputEventKind::DeviceAdded: {
            const bool restored = m_seenLogicalIds.contains(logicalId);
            m_seenLogicalIds.insert(logicalId);
            emit deviceConnected(logicalId, restored);
            break;
        }
        case GameInputEventKind::CapabilityChanged:
            // observeDevice already refreshed the attachment's capabilities
            // in the registry; surface the change to Settings/diagnostics.
            emit statusChanged();
            break;
        case GameInputEventKind::Wake:
            emit deviceConnected(logicalId, m_seenLogicalIds.contains(logicalId));
            m_seenLogicalIds.insert(logicalId);
            break;
        case GameInputEventKind::DeviceRemoved:
        case GameInputEventKind::Sleep: {
            const auto held = m_heldSystemControls.take(event.deviceId);
            for (const QString& control : held)
                emit systemControlReleased(control, logicalId, m_deviceNames.value(event.deviceId));
            m_deviceExtraStates.remove(event.deviceId);
            m_deviceExtraControls.remove(event.deviceId);
            m_deviceStandardButtons.remove(event.deviceId);
            for (const QString& control : routerRef().disconnect(logicalId)) {
                if (!held.contains(control))
                    emit systemControlReleased(control, logicalId,
                                               m_deviceNames.value(event.deviceId));
            }
            if (event.kind == GameInputEventKind::DeviceRemoved) {
                m_removedDevices.insert(event.deviceId);
                registryRef().removeProvider(ControllerProvider::GameInput, event.deviceId);
                emit deviceDisconnected(logicalId);
            }
            emit lifecycleReset(logicalId,
                                event.kind == GameInputEventKind::Sleep
                                    ? QStringLiteral("controller sleep")
                                    : QStringLiteral("controller disconnected"));
            break;
        }
        case GameInputEventKind::Reading:
            // Standard-control readings stay SHADOW-ONLY (Stage A): Sony Raw
            // Input, XInput and WinMM still own standard controls through
            // InputEngine's legacy arbitration and do not feed
            // PhysicalControllerRegistry yet, so routing them here too would
            // double-fire one physical press through two pipelines. Stage C
            // may only return once every legacy provider reports into the
            // same registry and cross-provider dedup is proven end to end
            // (plan t25). System Share/Guide and extra buttons are
            // GameInput-exclusive and keep routing.
            ++m_shadowReadingCount;
            m_deviceStandardButtons.insert(event.deviceId, event.standardButtons);
            if (!event.buttonStates.isEmpty()) {
                const auto layout = m_extraButtons->observe(
                    logicalId, event.buttonStates.size(), event.device.buttonLabels);
                auto previous = m_deviceExtraStates.value(event.deviceId);
                if (layout.changed) {
                    if (!m_layoutWarnings.contains(logicalId)) {
                        m_layoutWarnings.insert(logicalId);
                        emit statusChanged();
                    }
                    // Release the old layout's held controls through the
                    // capability router so its own held/generation state
                    // stays consistent, then forget the old index states —
                    // an old index must never lend its meaning to a new
                    // layout's button.
                    const auto oldControls = m_deviceExtraControls.value(event.deviceId);
                    const auto held = m_heldSystemControls.value(event.deviceId);
                    for (const QString& control : oldControls) {
                        if (held.contains(control))
                            publishEdge(event.deviceId, logicalId, control, false,
                                        ControllerCapability::ExtraControls,
                                        event.timestamp);
                    }
                    previous = QVector<quint8>(event.buttonStates.size(), 0);
                }
                // A changed layout routes nothing until the user reconfirms
                // it; state keeps tracking so confirmation resumes cleanly.
                if (!layout.needsReconfirmation) {
                    for (int index = 0; index < event.buttonStates.size(); ++index) {
                        const bool wasPressed = index < previous.size()
                            && previous.at(index) != 0;
                        const bool isPressed = event.buttonStates.at(index) != 0;
                        if (wasPressed == isPressed)
                            continue;
                        publishEdge(event.deviceId, logicalId,
                                    layout.controlIds.at(index), isPressed,
                                    ControllerCapability::ExtraControls,
                                    event.timestamp);
                    }
                }
                m_deviceExtraStates.insert(event.deviceId, event.buttonStates);
                m_deviceExtraControls.insert(event.deviceId, layout.controlIds);
            }
            break;
        case GameInputEventKind::SystemButtonPressed:
            if (event.controlId != ControlId::Capture && event.controlId != ControlId::Guide)
                break;
            publishEdge(event.deviceId, logicalId, event.controlId, true,
                        event.controlId == ControlId::Capture
                            ? ControllerCapability::SystemShare : ControllerCapability::Guide,
                        event.timestamp);
            break;
        case GameInputEventKind::SystemButtonReleased:
            publishEdge(event.deviceId, logicalId, event.controlId, false,
                        event.controlId == ControlId::Capture
                            ? ControllerCapability::SystemShare : ControllerCapability::Guide,
                        event.timestamp);
            break;
        case GameInputEventKind::RecoveryRequired:
            failSession(QStringLiteral("GameInput state became uncertain"));
            return;
        }
    }
}

QString GameInputRouter::controllerSummary() const
{
    QStringList rows;
    auto logicalControllers = registry().controllers();
    std::sort(logicalControllers.begin(), logicalControllers.end(),
              [](const auto& left, const auto& right) { return left.logicalId < right.logicalId; });
    for (const auto& logical : logicalControllers) {
        QStringList providers;
        for (const auto& attachment : logical.providers)
            providers.push_back(providerLabel(attachment.provider));
        providers.removeDuplicates();
        const auto descriptor = m_descriptors.value(logical.logicalId);
        rows.push_back(QStringLiteral("%1 · %2 · Share %3 · Guide %4 · %5 extra")
            .arg(logical.displayName.isEmpty() ? QStringLiteral("Controller") : logical.displayName,
                 providers.isEmpty() ? QStringLiteral("Disconnected") : providers.join(QStringLiteral(" + ")),
                 logical.capabilities().testFlag(ControllerCapability::SystemShare)
                     ? QStringLiteral("available") : QStringLiteral("not reported"),
                 logical.capabilities().testFlag(ControllerCapability::Guide)
                     ? QStringLiteral("available") : QStringLiteral("not reported"))
            .arg(descriptor.extraButtonCount));
    }
    return rows.isEmpty() ? QStringLiteral("No modern controller reported")
                          : rows.join(QStringLiteral("\n"));
}

QString GameInputRouter::compatibilityReport() const
{
    QStringList report{
        QStringLiteral("GameHQ controller compatibility report"),
        QStringLiteral("GameInput runtime: %1").arg(m_runtimeStatus),
        QStringLiteral("Modern support: %1").arg(m_mode == SupportMode::Off
                                                    ? QStringLiteral("Off")
                                                    : QStringLiteral("Auto"))
    };
    auto logicalControllers = registry().controllers();
    std::sort(logicalControllers.begin(), logicalControllers.end(),
              [](const auto& left, const auto& right) { return left.logicalId < right.logicalId; });
    for (const auto& logical : logicalControllers) {
        const auto descriptor = m_descriptors.value(logical.logicalId);
        QStringList providers;
        for (const auto& attachment : logical.providers)
            providers.push_back(providerLabel(attachment.provider));
        providers.removeDuplicates();
        report << QStringLiteral("Device: %1").arg(
                      logical.displayName.isEmpty() ? QStringLiteral("Controller") : logical.displayName)
               << QStringLiteral("Anonymous ID: %1").arg(logical.logicalId)
               << QStringLiteral("VID/PID: %1:%2")
                      .arg(descriptor.vendorId, 4, 16, QLatin1Char('0'))
                      .arg(descriptor.productId, 4, 16, QLatin1Char('0'))
               << QStringLiteral("Providers: %1").arg(
                      providers.isEmpty() ? QStringLiteral("Disconnected")
                                          : providers.join(QStringLiteral(", ")))
               << QStringLiteral("Share: %1; Guide: %2; Extra buttons: %3")
                      .arg(logical.capabilities().testFlag(ControllerCapability::SystemShare)
                               ? QStringLiteral("Available") : QStringLiteral("Not reported"),
                           logical.capabilities().testFlag(ControllerCapability::Guide)
                               ? QStringLiteral("Available") : QStringLiteral("Not reported"))
                      .arg(descriptor.extraButtonCount)
               << QStringLiteral("Layout reconfirmation: %1")
                      .arg(m_layoutWarnings.contains(logical.logicalId)
                               ? QStringLiteral("Required") : QStringLiteral("No"));
    }
    return report.join(QStringLiteral("\n"));
}

void GameInputRouter::publishEdge(const QString& deviceId, const QString& logicalId,
                                  const QString& controlId, bool pressed,
                                  ControllerCapability capability, quint64 timestamp)
{
    if (controlId.isEmpty())
        return;   // standard-labeled extras are filtered to an empty id
    // Cross-provider safety gate (t25): a Share/Guide press may reach us
    // twice — once here and once through a legacy provider (DualSense Sony
    // Raw Capture/Guide, XInput ordinal-100 Guide). When that legacy provider
    // is CORRELATED onto the same logical controller, the shared capability
    // router dedups the pair and exactly one edge survives. When a legacy
    // provider is live but NOT correlated onto this controller (ambiguous
    // identity: two same-model pads, missing fingerprint), dedup cannot be
    // guaranteed, so the GameInput edge stays shadowed — the proven legacy
    // path keeps sole ownership. GameInput-only controllers route freely.
    const bool systemCapability = capability == ControllerCapability::SystemShare
        || capability == ControllerCapability::Guide;
    if (systemCapability
        && m_integration->legacyProviderConnected()
        && !m_integration->hasLegacyAttachment(logicalId)) {
        const bool held = m_heldSystemControls.value(deviceId).contains(controlId);
        if (pressed || !held) {
            // A release of a control we still hold must pass through so the
            // action cannot stick; everything else is withheld.
            ++m_shadowedSystemEdges;
            return;
        }
    }
    const auto result = routerRef().route(
        {logicalId, ControllerProvider::GameInput, capability, controlId, pressed, timestamp});
    for (const QString& release : result.safeReleases) {
        m_heldSystemControls[deviceId].remove(release);
        emit systemControlReleased(release, logicalId, m_deviceNames.value(deviceId));
    }
    if (!result.accepted)
        return;
    if (pressed) {
        m_heldSystemControls[deviceId].insert(controlId);
        emit systemControlPressed(controlId, logicalId, m_deviceNames.value(deviceId));
    } else {
        m_heldSystemControls[deviceId].remove(controlId);
        emit systemControlReleased(controlId, logicalId, m_deviceNames.value(deviceId));
    }
}

int GameInputRouter::confirmLayouts()
{
    int confirmed = 0;
    const auto warned = m_layoutWarnings;
    for (const QString& logicalId : warned) {
        if (m_extraButtons->confirm(logicalId)) {
            m_layoutWarnings.remove(logicalId);
            ++confirmed;
        }
    }
    if (confirmed > 0)
        emit statusChanged();
    return confirmed;
}

void GameInputRouter::releaseHeldControls()
{
    for (auto it = m_heldSystemControls.cbegin(); it != m_heldSystemControls.cend(); ++it) {
        const QString logicalId = m_deviceLogicalIds.value(it.key());
        for (const QString& control : it.value())
            emit systemControlReleased(control, logicalId, m_deviceNames.value(it.key()));
    }
    m_heldSystemControls.clear();
}

void GameInputRouter::failSession(const QString& reason)
{
    if (m_failedForSession)
        return;
    releaseHeldControls();
    m_wrapper->shutdown();
    m_active = false;
    m_failedForSession = true;
    detachFromRegistry();
    m_runtimeStatus = QStringLiteral("Legacy fallback: %1").arg(reason);
    InputDiagnostics::instance().noteBackendSwitch(QStringLiteral("Legacy controllers"), reason);
    emit statusChanged();
    emit sessionFallback(reason);
}

} // namespace ModernInput
