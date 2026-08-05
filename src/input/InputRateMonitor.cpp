#include "input/InputRateMonitor.h"

void InputRateMonitor::record(void* handle, bool ignored)
{
    auto it = m_counters.find(handle);
    if (it == m_counters.end())
        it = m_counters.insert(handle, Counter{});   // once per handle, never per event
    ++it->events;
    it->ignored = ignored;
}

void InputRateMonitor::forget(void* handle)
{
    m_counters.remove(handle);
}

void InputRateMonitor::clear()
{
    m_counters.clear();
}

QList<InputRateMonitor::Sample> InputRateMonitor::sample(qint64 windowMs)
{
    QList<Sample> samples;
    if (windowMs <= 0)
        return samples;

    samples.reserve(m_counters.size());
    for (auto it = m_counters.begin(); it != m_counters.end();) {
        const quint64 rate64 = (it->events * 1000 + static_cast<quint64>(windowMs) / 2)
            / static_cast<quint64>(windowMs);
        const auto rate = static_cast<quint32>(qMin<quint64>(rate64, 0xFFFFFFFFull));

        bool worthLogging = false;
        if (rate == 0) {
            // Report the stop once, then let the entry go: a handle value that
            // Windows reuses after a replug must not inherit this history.
            worthLogging = it->lastLoggedRate > 0;
            it->lastLoggedRate = 0;
        } else if (it->lastLoggedRate == 0) {
            worthLogging = true;   // first traffic from this device
        } else {
            const quint32 previous = it->lastLoggedRate;
            const quint32 delta = rate > previous ? rate - previous : previous - rate;
            worthLogging = delta >= qMax(kMinimumInterestingDelta, previous / 4);
        }
        if (worthLogging && rate > 0)
            it->lastLoggedRate = rate;

        samples.append(Sample{ it.key(), rate, it->events, it->ignored, worthLogging });

        if (rate == 0 && it->lastLoggedRate == 0) {
            it = m_counters.erase(it);
            continue;
        }
        it->events = 0;
        ++it;
    }
    return samples;
}
