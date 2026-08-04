#include "gameinput/GameInputRouter.h"

#include "gameinput/GameInputWrapper.h"
#include "gameinput/IGameInputApi.h"
#include "input/ControlId.h"
#include "input/InputDiagnostics.h"
#include "input/ExtraButtonCatalog.h"

namespace ModernInput {

GameInputRouter::GameInputRouter(std::unique_ptr<IGameInputApi> api, SupportMode mode,
                                 CaptureDatabase* database,
                                 QObject* parent)
    : QObject(parent)
    , m_wrapper(std::make_unique<GameInputWrapper>(std::move(api)))
    , m_extraButtons(std::make_unique<ExtraButtonCatalog>(database))
    , m_mode(mode)
{
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
    InputDiagnostics::instance().noteDevice(
        QStringLiteral("%1:%2").arg(device.vendorId, 4, 16, QLatin1Char('0'))
                                  .arg(device.productId, 4, 16, QLatin1Char('0')),
        device.displayName, QStringLiteral("GameInput shadow; system buttons routed"));
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
            // Stage A: standard readings are diagnostic shadow traffic only.
            ++m_shadowReadingCount;
            if (!event.buttonStates.isEmpty()) {
                const auto layout = m_extraButtons->observe(
                    logicalId, event.buttonStates.size(), event.device.buttonLabels);
                const auto previous = m_deviceExtraStates.value(event.deviceId);
                const auto oldControls = m_deviceExtraControls.value(event.deviceId);
                if (layout.changed) {
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
                        emit systemControlPressed(control, logicalId,
                                                  m_deviceNames.value(event.deviceId));
                    } else {
                        m_heldSystemControls[event.deviceId].remove(control);
                        emit systemControlReleased(control, logicalId,
                                                   m_deviceNames.value(event.deviceId));
                    }
                }
                m_deviceExtraStates.insert(event.deviceId, event.buttonStates);
                m_deviceExtraControls.insert(event.deviceId, layout.controlIds);
            }
            break;
        case GameInputEventKind::SystemButtonPressed:
            if (event.controlId != ControlId::Capture && event.controlId != ControlId::Guide)
                break;
            m_heldSystemControls[event.deviceId].insert(event.controlId);
            emit systemControlPressed(event.controlId, logicalId,
                                      m_deviceNames.value(event.deviceId));
            break;
        case GameInputEventKind::SystemButtonReleased:
            m_heldSystemControls[event.deviceId].remove(event.controlId);
            emit systemControlReleased(event.controlId, logicalId,
                                       m_deviceNames.value(event.deviceId));
            break;
        case GameInputEventKind::RecoveryRequired:
            failSession(QStringLiteral("GameInput state became uncertain"));
            return;
        }
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
