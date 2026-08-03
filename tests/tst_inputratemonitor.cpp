#include <QtTest>

#include "input/InputRateMonitor.h"

// Pure logic: no Win32, no event loop. The handles are opaque keys, so plain
// integers cast to void* are as good as anything the OS would hand out.
namespace
{
void* handle(quintptr id)
{
    return reinterpret_cast<void*>(id);
}

InputRateMonitor::Sample sampleFor(const QList<InputRateMonitor::Sample>& samples, void* h)
{
    for (const auto& s : samples) {
        if (s.handle == h)
            return s;
    }
    return {};
}
} // namespace

class InputRateMonitorTest : public QObject
{
    Q_OBJECT

private slots:
    void rateIsEventsPerSecondOverTheSampledWindow()
    {
        InputRateMonitor monitor;
        for (int i = 0; i < 40000; ++i)
            monitor.record(handle(1), true);

        const auto samples = monitor.sample(5000);
        QCOMPARE(samples.size(), 1);
        QCOMPARE(samples.first().eventsPerSecond, 8000u);
        QCOMPARE(samples.first().events, 40000ull);
        QVERIFY(samples.first().ignored);
        QVERIFY(samples.first().worthLogging);   // first traffic is always news
    }

    void steadyTrafficIsNotLoggedTwice()
    {
        InputRateMonitor monitor;
        for (int i = 0; i < 1250; ++i)
            monitor.record(handle(1), false);
        QVERIFY(monitor.sample(5000).first().worthLogging);

        // Same pad, same rate, next window: nothing new to say.
        for (int i = 0; i < 1250; ++i)
            monitor.record(handle(1), false);
        const auto second = monitor.sample(5000);
        QCOMPARE(second.first().eventsPerSecond, 250u);
        QVERIFY(!second.first().worthLogging);
    }

    void jitterIsIgnoredButAMaterialChangeIsReported()
    {
        InputRateMonitor monitor;
        for (int i = 0; i < 1250; ++i)
            monitor.record(handle(1), false);
        QVERIFY(monitor.sample(5000).first().worthLogging);   // 250/s

        for (int i = 0; i < 1290; ++i)                        // 258/s — jitter
            monitor.record(handle(1), false);
        QVERIFY(!monitor.sample(5000).first().worthLogging);

        for (int i = 0; i < 40000; ++i)                       // 8000/s — news
            monitor.record(handle(1), false);
        QVERIFY(monitor.sample(5000).first().worthLogging);
    }

    void aStoppedStreamIsReportedOnceAndThenForgotten()
    {
        InputRateMonitor monitor;
        monitor.record(handle(1), true);
        QVERIFY(monitor.sample(1000).first().worthLogging);

        const auto stopped = monitor.sample(1000);
        QCOMPARE(stopped.size(), 1);
        QCOMPARE(stopped.first().eventsPerSecond, 0u);
        QVERIFY(stopped.first().worthLogging);

        // Dropped entirely, so a handle value Windows reuses after a replug
        // cannot inherit the previous device's history.
        QVERIFY(monitor.sample(1000).isEmpty());
    }

    void devicesAreCountedIndependently()
    {
        InputRateMonitor monitor;
        for (int i = 0; i < 40000; ++i)
            monitor.record(handle(1), true);
        for (int i = 0; i < 1250; ++i)
            monitor.record(handle(2), false);

        const auto samples = monitor.sample(5000);
        QCOMPARE(samples.size(), 2);
        QCOMPARE(sampleFor(samples, handle(1)).eventsPerSecond, 8000u);
        QVERIFY(sampleFor(samples, handle(1)).ignored);
        QCOMPARE(sampleFor(samples, handle(2)).eventsPerSecond, 250u);
        QVERIFY(!sampleFor(samples, handle(2)).ignored);
    }

    void forgottenHandlesLeaveNoTrace()
    {
        InputRateMonitor monitor;
        monitor.record(handle(1), true);
        monitor.forget(handle(1));
        QVERIFY(monitor.sample(1000).isEmpty());
    }

    void aZeroWindowCannotDivideByZero()
    {
        InputRateMonitor monitor;
        monitor.record(handle(1), true);
        QVERIFY(monitor.sample(0).isEmpty());
        // The counter survives: nothing was sampled, so nothing was reset.
        QCOMPARE(monitor.sample(1000).first().events, 1ull);
    }
};

QTEST_APPLESS_MAIN(InputRateMonitorTest)
#include "tst_inputratemonitor.moc"
