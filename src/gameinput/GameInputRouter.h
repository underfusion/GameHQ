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
    SupportMode mode() const { return m_mode; }
    bool active() const { return m_active; }
    bool failedForSession() const { return m_failedForSession; }
    QString runtimeStatus() const { return m_runtimeStatus; }
    int shadowReadingCount() const { return m_shadowReadingCount; }
    const PhysicalControllerRegistry& registry() const { return m_registry; }
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
    void releaseHeldControls();
    void failSession(const QString& reason);
    void publishEdge(const QString& deviceId, const QString& logicalId,
                     const QString& controlId, bool pressed,
                     ControllerCapability capability, quint64 timestamp);

    std::unique_ptr<GameInputWrapper> m_wrapper;
    std::unique_ptr<ExtraButtonCatalog> m_extraButtons;
    PhysicalControllerRegistry m_registry;
    std::unique_ptr<CapabilityEventRouter> m_capabilityRouter;
    SupportMode m_mode = SupportMode::Auto;
    bool m_active = false;
    bool m_failedForSession = false;
    QString m_runtimeStatus = QStringLiteral("Not started");
    int m_shadowReadingCount = 0;
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
