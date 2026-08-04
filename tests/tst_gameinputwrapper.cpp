#include "gameinput/FakeGameInputApi.h"
#include "gameinput/GameInputEventQueue.h"
#include "gameinput/GameInputWrapper.h"

#include <QCoreApplication>
#include <QSignalSpy>
#include <QTest>
#include <QThread>

using namespace ModernInput;

class TestGameInputWrapper : public QObject
{
    Q_OBJECT

private slots:
    void discreteOrderAndStateCoalescing();
    void quickTapSurvivesOneDrain();
    void staleReadingIsDroppedWhenDeviceRemoved();
    void overflowRequestsRecovery();
    void callbacksArriveOnQtThread();
    void shutdownIsOrderedAndLateCallbacksAreHarmless();
    void restartAfterShutdownDeliversEventsAgain();
    void initializationFailureFailsSoft();
};

void TestGameInputWrapper::discreteOrderAndStateCoalescing()
{
    GameInputEventQueue queue(8, 4);
    GameInputEvent state1;
    state1.kind = GameInputEventKind::Reading;
    state1.deviceId = QStringLiteral("pad-a");
    state1.standardButtons = 1;
    // First reading is a button transition (unknown -> 1): ordered/discrete.
    QVERIFY(queue.push(state1));
    // Button-stable repeats coalesce into one latest-state entry.
    QVERIFY(queue.push(state1));
    QVERIFY(queue.push(state1));

    for (int i = 0; i < 3; ++i) {
        GameInputEvent edge;
        edge.kind = i % 2 ? GameInputEventKind::SystemButtonReleased
                          : GameInputEventKind::SystemButtonPressed;
        edge.deviceId = QStringLiteral("pad-a");
        edge.controlId = QStringLiteral("control-%1").arg(i);
        QVERIFY(queue.push(edge));
    }

    const GameInputEventBatch batch = queue.take();
    QCOMPARE(batch.events.size(), 5);
    QCOMPARE(batch.events[0].kind, GameInputEventKind::Reading);
    QCOMPARE(batch.events[0].standardButtons, quint32(1));
    QCOMPARE(batch.events[1].controlId, QStringLiteral("control-0"));
    QCOMPARE(batch.events[2].controlId, QStringLiteral("control-1"));
    QCOMPARE(batch.events[3].controlId, QStringLiteral("control-2"));
    QCOMPARE(batch.events[4].kind, GameInputEventKind::Reading);
    QVERIFY(batch.events[1].sequence < batch.events[2].sequence);
    QVERIFY(batch.events[2].sequence < batch.events[3].sequence);
    QVERIFY(!batch.forceResync);
}

void TestGameInputWrapper::quickTapSurvivesOneDrain()
{
    // A press-then-release arriving before a single drain must deliver both
    // edges in order — coalescing here would erase the whole tap.
    GameInputEventQueue queue(8, 4);
    GameInputEvent reading;
    reading.kind = GameInputEventKind::Reading;
    reading.deviceId = QStringLiteral("pad-tap");
    reading.buttonStates = {0, 0, 0};
    QVERIFY(queue.push(reading));
    reading.buttonStates = {0, 0, 1};
    QVERIFY(queue.push(reading));
    reading.buttonStates = {0, 0, 0};
    QVERIFY(queue.push(reading));

    const GameInputEventBatch batch = queue.take();
    QCOMPARE(batch.events.size(), 3);
    QCOMPARE(batch.events[0].buttonStates, (QVector<quint8>{0, 0, 0}));
    QCOMPARE(batch.events[1].buttonStates, (QVector<quint8>{0, 0, 1}));
    QCOMPARE(batch.events[2].buttonStates, (QVector<quint8>{0, 0, 0}));
    QVERIFY(batch.events[0].sequence < batch.events[1].sequence);
    QVERIFY(batch.events[1].sequence < batch.events[2].sequence);
    QVERIFY(!batch.forceResync);
}

void TestGameInputWrapper::staleReadingIsDroppedWhenDeviceRemoved()
{
    GameInputEventQueue queue(8, 4);
    GameInputEvent reading;
    reading.kind = GameInputEventKind::Reading;
    reading.deviceId = QStringLiteral("pad-gone");
    reading.standardButtons = 1;
    QVERIFY(queue.push(reading)); // discrete transition
    QVERIFY(queue.push(reading)); // coalesced latest state

    GameInputEvent removed;
    removed.kind = GameInputEventKind::DeviceRemoved;
    removed.deviceId = reading.deviceId;
    QVERIFY(queue.push(removed));

    // The pending coalesced reading predates the removal; delivering it after
    // DeviceRemoved would let the consumer re-observe a dead device.
    const GameInputEventBatch batch = queue.take();
    QCOMPARE(batch.events.size(), 2);
    QCOMPARE(batch.events[0].kind, GameInputEventKind::Reading);
    QCOMPARE(batch.events[1].kind, GameInputEventKind::DeviceRemoved);
}

void TestGameInputWrapper::overflowRequestsRecovery()
{
    GameInputEventQueue queue(2, 2);
    for (int i = 0; i < 5; ++i) {
        GameInputEvent edge;
        edge.kind = i % 2 ? GameInputEventKind::SystemButtonReleased
                          : GameInputEventKind::SystemButtonPressed;
        edge.deviceId = QStringLiteral("pad-overflow");
        edge.controlId = QStringLiteral("gamepad.extra.%1").arg(i);
        QVERIFY(queue.push(edge));
    }
    const GameInputEventBatch batch = queue.take();
    QVERIFY(batch.forceResync);
    QVERIFY(batch.overflowCount >= 1);
    QCOMPARE(batch.uncertainDevices, QStringList{QStringLiteral("pad-overflow")});
    QCOMPARE(batch.events.back().kind, GameInputEventKind::RecoveryRequired);
    QCOMPARE(batch.events.back().deviceId, QStringLiteral("pad-overflow"));
}

void TestGameInputWrapper::callbacksArriveOnQtThread()
{
    auto fake = std::make_unique<FakeGameInputApi>();
    FakeGameInputApi* raw = fake.get();
    GameInputWrapper wrapper(std::move(fake));
    QString error;
    QVERIFY2(wrapper.start(error), qPrintable(error));

    bool deliveredOnQtThread = false;
    GameInputEventBatch delivered;
    connect(&wrapper, &GameInputWrapper::eventsReady, this,
            [&](const GameInputEventBatch& batch) {
        deliveredOnQtThread = QThread::currentThread() == qApp->thread();
        delivered = batch;
    });

    GameInputEvent event;
    event.kind = GameInputEventKind::SystemButtonPressed;
    event.deviceId = QStringLiteral("pad-thread");
    event.controlId = QStringLiteral("gamepad.capture");
    raw->emitSystem(event, true);

    QTRY_VERIFY_WITH_TIMEOUT(!delivered.events.isEmpty(), 2000);
    QVERIFY(deliveredOnQtThread);
    QCOMPARE(delivered.events.front().controlId, QStringLiteral("gamepad.capture"));
}

void TestGameInputWrapper::shutdownIsOrderedAndLateCallbacksAreHarmless()
{
    auto fake = std::make_unique<FakeGameInputApi>();
    FakeGameInputApi* raw = fake.get();
    GameInputWrapper wrapper(std::move(fake));
    QString error;
    QVERIFY(wrapper.start(error));
    QSignalSpy delivered(&wrapper, &GameInputWrapper::eventsReady);

    wrapper.shutdown();
    const QStringList log = raw->callLog();
    const int release = log.indexOf(QStringLiteral("release-devices"));
    const int unload = log.indexOf(QStringLiteral("unload"));
    QVERIFY(release > 0);
    QVERIFY(unload > release);
    for (int token = 1; token <= 3; ++token) {
        const int stop = log.indexOf(QStringLiteral("stop:%1").arg(token));
        const int unregister = log.indexOf(QStringLiteral("unregister:%1").arg(token));
        QVERIFY(stop > 0);
        QVERIFY(unregister > stop);
        QVERIFY(unregister < release);
    }

    GameInputEvent late;
    late.kind = GameInputEventKind::SystemButtonReleased;
    late.deviceId = QStringLiteral("late-pad");
    raw->emitRetired(late);
    QCoreApplication::processEvents();
    QCOMPARE(delivered.count(), 0);
    QVERIFY(!wrapper.running());
}

void TestGameInputWrapper::restartAfterShutdownDeliversEventsAgain()
{
    // The Settings toggle is Auto -> Off -> Auto: a wrapper that was shut
    // down must deliver events again after the next start().
    auto fake = std::make_unique<FakeGameInputApi>();
    FakeGameInputApi* raw = fake.get();
    GameInputWrapper wrapper(std::move(fake));
    QSignalSpy delivered(&wrapper, &GameInputWrapper::eventsReady);
    QString error;

    for (int cycle = 0; cycle < 2; ++cycle) {
        QVERIFY2(wrapper.start(error), qPrintable(error));
        const int before = delivered.count();
        GameInputEvent event;
        event.kind = GameInputEventKind::SystemButtonPressed;
        event.deviceId = QStringLiteral("pad-restart");
        event.controlId = QStringLiteral("gamepad.capture");
        raw->emitSystem(event, true);
        QTRY_VERIFY_WITH_TIMEOUT(delivered.count() > before, 2000);
        wrapper.shutdown();
        QVERIFY(!wrapper.running());
    }
}

void TestGameInputWrapper::initializationFailureFailsSoft()
{
    auto fake = std::make_unique<FakeGameInputApi>();
    fake->setInitializeFailure(QStringLiteral("runtime missing"));
    GameInputWrapper wrapper(std::move(fake));
    QString error;
    QVERIFY(!wrapper.start(error));
    QCOMPARE(error, QStringLiteral("runtime missing"));
    QVERIFY(!wrapper.running());
}

QTEST_GUILESS_MAIN(TestGameInputWrapper)
#include "tst_gameinputwrapper.moc"

