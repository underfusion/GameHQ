#pragma once

#include "gameinput/GameInputQtDispatcher.h"
#include "gameinput/IGameInputApi.h"

#include <QObject>

#include <memory>

namespace ModernInput {

class GameInputWrapper final : public QObject
{
    Q_OBJECT
public:
    explicit GameInputWrapper(std::unique_ptr<IGameInputApi> api,
                              int queueCapacity = 256,
                              int emergencyReserve = 64,
                              QObject* parent = nullptr);
    ~GameInputWrapper() override;

    bool start(QString& error);
    void shutdown();
    bool running() const { return m_running; }
    QString runtimeDescription() const;

signals:
    void eventsReady(const ModernInput::GameInputEventBatch& batch);

private:
    std::unique_ptr<IGameInputApi> m_api;
    std::shared_ptr<GameInputEventQueue> m_queue;
    std::unique_ptr<GameInputQtDispatcher> m_dispatcher;
    GameInputCallbackRegistration m_deviceRegistration;
    GameInputCallbackRegistration m_readingRegistration;
    GameInputCallbackRegistration m_systemRegistration;
    bool m_running = false;
};

} // namespace ModernInput

