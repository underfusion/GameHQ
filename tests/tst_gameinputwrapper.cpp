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
    void overflowRequestsRecovery();
    void callbacksArriveOnQtThread();
    void shutdownIsOrderedAndLateCallbacksAreHarmless();
    void initializationFailureFailsSoft();
};

void TestGameInputWrapper::discreteOrderAndStateCoalescing()
{
    GameInputEventQueue queue(8, 4);
    GameInputEvent state1;
    state1.kind = GameInputEventKind::Reading;
    state1.deviceId = QStringLiteral("pad-a");
    state1.standardButtons = 1;
    QVERIFY(queue.push(state1));
    state1.standardButtons = 7;
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
    QCOMPARE(batch.events.size(), 4);
    QCOMPARE(batch.events[0].controlId, QStringLiteral("control-0"));
    QCOMPARE(batch.events[1].controlId, QStringLiteral("control-1"));
    QCOMPARE(batch.events[2].controlId, QStringLiteral("control-2"));
    QCOMPARE(batch.events[3].standardButtons, quint32(7));
    QVERIFY(batch.events[0].sequence < batch.events[1].sequence);
    QVERIFY(batch.events[1].sequence < batch.events[2].sequence);
    QVERIFY(!batch.forceResync);
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

