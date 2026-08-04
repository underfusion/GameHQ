#include "gameinput/GameInputRouter.h"

#include "gameinput/GameInputWrapper.h"
#include "gameinput/IGameInputApi.h"
#include "input/ControlId.h"
#include "input/InputDiagnostics.h"
#include "input/ExtraButtonCatalog.h"
#include "input/CapabilityEventRouter.h"

#include <algorithm>

namespace ModernInput {

namespace {
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
    m_capabilityRouter = std::make_unique<CapabilityEventRouter>(&m_registry);
    connect(m_wrapper.get(), &GameInputWrapper::eventsReady,
            this, &GameInputRouter::handleBatch);
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
    if (m_mode == SupportMode::Off)
        m_runtimeStatus = QStringLiteral("Off");
    emit statusChanged();
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
    observation.capabilities = ControllerCapability::StandardControls;
    if (device.supportedSystemButtons != 0)
        observation.capabilities |= ControllerCapability::SystemShare
            | ControllerCapability::Guide;
    if (device.extraButtonCount > 0)
        observation.capabilities |= ControllerCapability::ExtraControls;
    const QString logicalId = m_registry.observe(observation);
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
            for (const QString& control : m_capabilityRouter->disconnect(logicalId)) {
                if (!held.contains(control))
                    emit systemControlReleased(control, logicalId,
                                               m_deviceNames.value(event.deviceId));
            }
            if (event.kind == GameInputEventKind::DeviceRemoved) {
                m_registry.removeProvider(ControllerProvider::GameInput, event.deviceId);
                emit deviceDisconnected(logicalId);
            }
            emit lifecycleReset(logicalId,
                                event.kind == GameInputEventKind::Sleep
                                    ? QStringLiteral("controller sleep")
                                    : QStringLiteral("controller disconnected"));
            break;
        }
        case GameInputEventKind::Reading:
            // Stage A diagnostics remain counted; after the t25 acceptance
            // gate, Stage C also publishes preferred standard-control edges.
            ++m_shadowReadingCount;
            {
                struct StandardMapping { quint32 mask; const QString* control; };
                const StandardMapping mappings[] = {
                    {0x00000001u, &ControlId::Menu}, {0x00000002u, &ControlId::ViewBack},
                    {0x00000004u, &ControlId::FaceSouth}, {0x00000008u, &ControlId::FaceEast},
                    {0x00000010u, &ControlId::FaceWest}, {0x00000020u, &ControlId::FaceNorth},
                    {0x00000040u, &ControlId::DpadUp}, {0x00000080u, &ControlId::DpadDown},
                    {0x00000100u, &ControlId::DpadLeft}, {0x00000200u, &ControlId::DpadRight},
                    {0x00000400u, &ControlId::ShoulderLeft}, {0x00000800u, &ControlId::ShoulderRight},
                    {0x00010000u, &ControlId::TriggerLeft}, {0x00020000u, &ControlId::TriggerRight},
                };
                const quint32 previousButtons = m_deviceStandardButtons.value(event.deviceId);
                const quint32 changed = previousButtons ^ event.standardButtons;
                for (const auto& mapping : mappings) {
                    if ((changed & mapping.mask) == 0)
                        continue;
                    publishEdge(event.deviceId, logicalId, *mapping.control,
                                (event.standardButtons & mapping.mask) != 0,
                                ControllerCapability::StandardControls, event.timestamp);
                }
                m_deviceStandardButtons.insert(event.deviceId, event.standardButtons);
            }
            if (!event.buttonStates.isEmpty()) {
                const auto layout = m_extraButtons->observe(
                    logicalId, event.buttonStates.size(), event.device.buttonLabels);
                const auto previous = m_deviceExtraStates.value(event.deviceId);
                const auto oldControls = m_deviceExtraControls.value(event.deviceId);
                if (layout.changed) {
                    if (!m_layoutWarnings.contains(logicalId)) {
                        m_layoutWarnings.insert(logicalId);
                        emit statusChanged();
                    }
                    const auto held = m_heldSystemControls.value(event.deviceId);
                    for (const QString& control : oldControls) {
                        if (held.contains(control))
                            emit systemControlReleased(control, logicalId,
                                                       m_deviceNames.value(event.deviceId));
                        m_heldSystemControls[event.deviceId].remove(control);
                    }
                }
                for (int index = 0; index < event.buttonStates.size(); ++index) {
                    const bool wasPressed = index < previous.size() && previous.at(index) != 0;
                    const bool isPressed = event.buttonStates.at(index) != 0;
                    if (wasPressed == isPressed)
                        continue;
                    const QString control = layout.controlIds.at(index);
                    if (isPressed) {
                        m_heldSystemControls[event.deviceId].insert(control);
                        publishEdge(event.deviceId, logicalId, control, true,
                                    ControllerCapability::ExtraControls, event.timestamp);
                    } else {
                        m_heldSystemControls[event.deviceId].remove(control);
                        publishEdge(event.deviceId, logicalId, control, false,
                                    ControllerCapability::ExtraControls, event.timestamp);
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
    auto logicalControllers = m_registry.controllers();
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
    auto logicalControllers = m_registry.controllers();
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
    const auto result = m_capabilityRouter->route(
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
    m_runtimeStatus = QStringLiteral("Legacy fallback: %1").arg(reason);
    InputDiagnostics::instance().noteBackendSwitch(QStringLiteral("Legacy controllers"), reason);
    emit statusChanged();
    emit sessionFallback(reason);
}

} // namespace ModernInput
