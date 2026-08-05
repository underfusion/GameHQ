#pragma once
#include <QHash>
#include <QList>
#include <QtGlobal>

// Aggregated per-device Raw Input event-rate accounting.
//
// A pad at 8000 Hz produces 8000 log lines per second if anything logs per
// event, which is worse than the flood it is meant to diagnose. So the hot
// path only increments a counter (one hash lookup, no allocation once the
// handle is known), and the backend samples the counters on an interval.
//
// A line every interval would still be noise during a normal session, so a
// sample is only "worth logging" when it says something new: the first traffic
// from a device, a material change in its rate, or the stream stopping. Pure
// logic, no Win32 and no Qt event loop — see tests/tst_inputratemonitor.cpp.
class InputRateMonitor
{
public:
    struct Sample {
        void* handle = nullptr;
        quint32 eventsPerSecond = 0;
        quint64 events = 0;         // raw count within the sampled window
        bool ignored = false;       // device is classified as "not ours"
        bool worthLogging = false;  // materially different from the last logged rate
    };

    // Below this many events per second a change is treated as jitter. Without
    // it a pad idling around 250 Hz would log on every sample.
    static constexpr quint32 kMinimumInterestingDelta = 50;

    void record(void* handle, bool ignored);
    void forget(void* handle);
    void clear();

    // Converts the counters accumulated over `windowMs` into per-device rates,
    // decides which ones are worth a log line, and resets for the next window.
    // Handles that went quiet and had nothing left to report are dropped.
    QList<Sample> sample(qint64 windowMs);

private:
    struct Counter {
        quint64 events = 0;
        quint32 lastLoggedRate = 0;
        bool ignored = false;
    };

    QHash<void*, Counter> m_counters;
};
