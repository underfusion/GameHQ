#include "gameinput/GameInputWrapper.h"

namespace ModernInput {

GameInputWrapper::GameInputWrapper(std::unique_ptr<IGameInputApi> api,
                                   int queueCapacity, int emergencyReserve,
                                   QObject* parent)
    : QObject(parent)
    , m_api(std::move(api))
    , m_queueCapacity(queueCapacity)
    , m_emergencyReserve(emergencyReserve)
    , m_queue(std::make_shared<GameInputEventQueue>(queueCapacity, emergencyReserve))
{
}

GameInputWrapper::~GameInputWrapper()
{
    shutdown();
}

QString GameInputWrapper::runtimeDescription() const
{
    return m_api ? m_api->runtimeDescription() : QStringLiteral("Unavailable");
}

bool GameInputWrapper::start(QString& error)
{
    if (m_running)
        return true;
    if (!m_api || !m_api->initialize(error))
        return false;

    // A previous shutdown() permanently stops the old queue so late callbacks
    // stay harmless. A restart (Auto → Off → Auto) therefore needs a fresh
    // queue; stale callbacks still hold only the old, stopped instance.
    if (!m_queue->accepting())
        m_queue = std::make_shared<GameInputEventQueue>(m_queueCapacity,
                                                        m_emergencyReserve);

    m_dispatcher = std::make_unique<GameInputQtDispatcher>(m_queue);
    connect(m_dispatcher.get(), &GameInputQtDispatcher::eventsReady,
            this, &GameInputWrapper::eventsReady);
    m_dispatcher->start();

    const std::weak_ptr<GameInputEventQueue> weakQueue(m_queue);
    const auto sink = [weakQueue](GameInputEvent event) {
        if (auto queue = weakQueue.lock())
            queue->push(std::move(event));
    };

    m_deviceRegistration = {m_api.get(), m_api->registerDeviceCallback(sink)};
    m_readingRegistration = {m_api.get(), m_api->registerReadingCallback(sink)};
    m_systemRegistration = {m_api.get(), m_api->registerSystemButtonCallback(sink)};
    if (!m_deviceRegistration.valid() || !m_readingRegistration.valid()
        || !m_systemRegistration.valid()) {
        error = QStringLiteral("GameInput callback registration failed.");
        shutdown();
        return false;
    }
    m_running = true;
    return true;
}

void GameInputWrapper::shutdown()
{
    if (!m_api)
        return;

    // Required order: stop/unregister every callback, release retained device
    // state, stop the queue bridge, then release/unload the runtime.
    m_systemRegistration.reset();
    m_readingRegistration.reset();
    m_deviceRegistration.reset();
    m_api->releaseDevices();
    if (m_dispatcher) {
        m_dispatcher->shutdown();
        m_dispatcher.reset();
    } else {
        m_queue->stopAccepting();
    }
    m_api->unload();
    m_running = false;
}

} // namespace ModernInput
