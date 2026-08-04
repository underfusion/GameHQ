#pragma once

#include "gameinput/GameInputEvent.h"

#include <QHash>
#include <QMutex>
#include <QQueue>
#include <QSet>
#include <QWaitCondition>

#include <atomic>

namespace ModernInput {

// Multi-producer/single-consumer queue used by GameInput callback threads.
// Discrete events retain arrival order. Readings are latest-state-only per
// logical device and never consume the emergency reserve.
class GameInputEventQueue
{
public:
    explicit GameInputEventQueue(int normalCapacity = 256, int emergencyReserve = 64);

    bool push(GameInputEvent event);
    GameInputEventBatch take();
    GameInputEventBatch waitTake();

    void stopAccepting();
    bool accepting() const { return m_accepting.load(std::memory_order_acquire); }
    int pendingCount() const;

private:
    GameInputEventBatch takeLocked();
    void noteOverflowLocked(const QString& deviceId);

    const int m_normalCapacity;
    const int m_emergencyReserve;
    mutable QMutex m_mutex;
    QWaitCondition m_available;
    QQueue<GameInputEvent> m_discrete;
    QHash<QString, GameInputEvent> m_latestStates;
    QSet<QString> m_uncertainDevices;
    quint64 m_nextSequence = 1;
    quint64 m_overflowCount = 0;
    bool m_forceResync = false;
    std::atomic_bool m_accepting{true};
};

} // namespace ModernInput

