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

    std::unique_ptr<GameInputWrapper> m_wrapper;
    std::unique_ptr<ExtraButtonCatalog> m_extraButtons;
    PhysicalControllerRegistry m_registry;
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
    QSet<QString> m_seenLogicalIds;
};

} // namespace ModernInput
