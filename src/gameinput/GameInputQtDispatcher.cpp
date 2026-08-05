#include "gameinput/GameInputQtDispatcher.h"

#include <QMetaObject>

namespace ModernInput {

GameInputQtDispatcher::GameInputQtDispatcher(
    std::shared_ptr<GameInputEventQueue> queue, QObject* parent)
    : QObject(parent), m_queue(std::move(queue))
{
    qRegisterMetaType<GameInputEventBatch>();
}

GameInputQtDispatcher::~GameInputQtDispatcher()
{
    shutdown();
}

void GameInputQtDispatcher::start()
{
    bool expected = false;
    if (!m_running.compare_exchange_strong(expected, true))
        return;
    m_worker = std::thread([this] { workerLoop(); });
}

void GameInputQtDispatcher::shutdown()
{
    if (!m_running.exchange(false))
        return;
    m_queue->stopAccepting();
    if (m_worker.joinable())
        m_worker.join();
}

void GameInputQtDispatcher::workerLoop()
{
    while (m_running.load(std::memory_order_acquire)) {
        GameInputEventBatch batch = m_queue->waitTake();
        if (batch.events.isEmpty() && !batch.forceResync)
            continue;
        QMetaObject::invokeMethod(this, [this, batch = std::move(batch)] {
            if (m_running.load(std::memory_order_acquire))
                emit eventsReady(batch);
        }, Qt::QueuedConnection);
    }
}

} // namespace ModernInput

