#pragma once

#include "gameinput/GameInputEvent.h"
#include "input/PhysicalControllerRegistry.h"

#include <QObject>
#include <QHash>
#include <QSet>

#include <memory>

class CaptureDatabase;

namespace ModernInput {

class GameInputWrapper;
class IGameInputApi;
class ExtraButtonCatalog;
class CapabilityEventRouter;
class ProviderIntegration;

class GameInputRouter final : public QObject
{
    Q_OBJECT
public:
    enum class SupportMode { Auto, Off };

    explicit GameInputRouter(std::unique_ptr<IGameInputApi> api,
                             SupportMode mode = SupportMode::Auto,
                             CaptureDatabase* database = nullptr,
                             QObject* parent = nullptr);
    ~GameInputRouter() override;

    bool start();
    void shutdown();
    void setMode(SupportMode mode);
    // t25 integration seam: share one PhysicalControllerRegistry and
    // CapabilityEventRouter with every legacy provider so a physical Share or
    // Guide press can never fire once through GameInput and once through Sony
    // Raw/XInput/WinMM. Must be installed before start(); without it the
    // router falls back to a private integration (GameInput-only dedup).
    void setProviderIntegration(ProviderIntegration* integration);
    // Number of system-button edges withheld because an uncorrelated legacy
    // provider was live (dedup could not be guaranteed). Diagnostics only.
    int shadowedSystemEdgeCount() const { return m_shadowedSystemEdges; }
    // The user confirmed the currently observed extra-button layout of every
    // controller carrying a layout warning. Returns how many were confirmed.
    int confirmLayouts();
    SupportMode mode() const { return m_mode; }
    bool active() const { return m_active; }
    bool failedForSession() const { return m_failedForSession; }
    QString runtimeStatus() const { return m_runtimeStatus; }
    int shadowReadingCount() const { return m_shadowReadingCount; }
    const PhysicalControllerRegistry& registry() const;
    QString controllerSummary() const;
    QString compatibilityReport() const;
    bool layoutWarning() const { return !m_layoutWarnings.isEmpty(); }

signals:
    void systemControlPressed(const QString& controlId, const QString& logicalId,
                              const QString& displayName);
    void systemControlReleased(const QString& controlId, const QString& logicalId,
                               const QString& displayName);
    void statusChanged();
    void sessionFallback(const QString& reason);
    void lifecycleReset(const QString& logicalId, const QString& reason);
    void deviceConnected(const QString& logicalId, bool profileRestored);
    void deviceDisconnected(const QString& logicalId);

private:
    void handleBatch(const GameInputEventBatch& batch);
    QString observeDevice(const GameInputDeviceDescriptor& device);
    void detachFromRegistry(const QString& reason);
    void failSession(const QString& reason);
    void publishEdge(const QString& deviceId, const QString& logicalId,
                     const QString& controlId, bool pressed,
                     ControllerCapability capability, quint64 timestamp);
    PhysicalControllerRegistry& registryRef();
    CapabilityEventRouter& routerRef();

    std::unique_ptr<GameInputWrapper> m_wrapper;
    std::unique_ptr<ExtraButtonCatalog> m_extraButtons;
    // Shared t25 integration when installed, private fallback otherwise. The
    // registry and capability router are always reached through these.
    ProviderIntegration* m_integration = nullptr;
    std::unique_ptr<ProviderIntegration> m_ownedIntegration;
    SupportMode m_mode = SupportMode::Auto;
    bool m_active = false;
    bool m_failedForSession = false;
    QString m_runtimeStatus = QStringLiteral("Not started");
    int m_shadowReadingCount = 0;
    int m_shadowedSystemEdges = 0;
    QHash<QString, QSet<QString>> m_heldSystemControls;
    QHash<QString, QString> m_deviceLogicalIds;
    QHash<QString, QString> m_deviceNames;
    QHash<QString, QVector<quint8>> m_deviceExtraStates;
    QHash<QString, QStringList> m_deviceExtraControls;
    QHash<QString, quint32> m_deviceStandardButtons;
    QSet<QString> m_seenLogicalIds;
    QSet<QString> m_removedDevices;
    QHash<QString, GameInputDeviceDescriptor> m_descriptors;
    QSet<QString> m_layoutWarnings;
};

} // namespace ModernInput
