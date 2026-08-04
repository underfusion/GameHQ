#include "gameinput/FakeGameInputApi.h"
#include "gameinput/GameInputRouter.h"
#include "input/ControlId.h"

#include <QSignalSpy>
#include <QtTest>

using namespace ModernInput;

namespace {
GameInputEvent makeEvent(GameInputEventKind kind, const QString& control = {})
{
    GameInputEvent result;
    result.kind = kind;
    result.deviceId = QStringLiteral("device-a");
    result.controlId = control;
    result.device.deviceId = result.deviceId;
    result.device.containerId = QStringLiteral("container-a");
    result.device.displayName = QStringLiteral("Modern test pad");
    result.device.vendorId = 0x1234;
    result.device.productId = 0x5678;
    result.device.supportedSystemButtons = 3;
    return result;
}
}

class GameInputRouterTest : public QObject
{
    Q_OBJECT

private slots:
    void offNeverLoadsTheRuntime()
    {
        auto api = std::make_unique<FakeGameInputApi>();
        auto* raw = api.get();
        GameInputRouter router(std::move(api), GameInputRouter::SupportMode::Off);
        QVERIFY(router.start());
        QCOMPARE(router.runtimeStatus(), QStringLiteral("Off"));
        QVERIFY(raw->callLog().isEmpty());
    }

    void readingsStayShadowWhileSystemButtonsRoute()
    {
        auto api = std::make_unique<FakeGameInputApi>();
        auto* raw = api.get();
        GameInputRouter router(std::move(api));
        QSignalSpy pressed(&router, &GameInputRouter::systemControlPressed);
        QSignalSpy released(&router, &GameInputRouter::systemControlReleased);
        QVERIFY(router.start());

        raw->emitDevice(makeEvent(GameInputEventKind::DeviceAdded));
        raw->emitReading(makeEvent(GameInputEventKind::Reading));
        QTRY_COMPARE(router.shadowReadingCount(), 1);
        QCOMPARE(pressed.size(), 0);

        raw->emitSystem(makeEvent(GameInputEventKind::SystemButtonPressed, ControlId::Capture));
        raw->emitSystem(makeEvent(GameInputEventKind::SystemButtonReleased, ControlId::Capture));
        QTRY_COMPARE(pressed.size(), 1);
        QTRY_COMPARE(released.size(), 1);
        QCOMPARE(pressed.at(0).at(0).toString(), ControlId::Capture);
        QVERIFY(!pressed.at(0).at(1).toString().isEmpty());
    }

    void uncertainStateReleasesHeldControlsAndFallsBack()
    {
        auto api = std::make_unique<FakeGameInputApi>();
        auto* raw = api.get();
        GameInputRouter router(std::move(api));
        QSignalSpy released(&router, &GameInputRouter::systemControlReleased);
        QSignalSpy fallback(&router, &GameInputRouter::sessionFallback);
        QVERIFY(router.start());
        raw->emitSystem(makeEvent(GameInputEventKind::SystemButtonPressed, ControlId::Guide));
        QTRY_VERIFY(router.active());
        raw->emitDevice(makeEvent(GameInputEventKind::RecoveryRequired));

        QTRY_COMPARE(fallback.size(), 1);
        QCOMPARE(released.size(), 1);
        QVERIFY(router.failedForSession());
        QVERIFY(!router.active());
        QVERIFY(raw->callLog().contains(QStringLiteral("unload")));
    }

    void extraButtonAbove32RoutesWithAStableDeviceLocalId()
    {
        auto api = std::make_unique<FakeGameInputApi>();
        auto* raw = api.get();
        GameInputRouter router(std::move(api));
        QSignalSpy pressed(&router, &GameInputRouter::systemControlPressed);
        QSignalSpy released(&router, &GameInputRouter::systemControlReleased);
        QVERIFY(router.start());

        auto reading = makeEvent(GameInputEventKind::Reading);
        reading.buttonStates.fill(0, 40);
        reading.device.extraButtonCount = 40;
        raw->emitReading(reading);
        QTRY_COMPARE(router.shadowReadingCount(), 1);
        reading.buttonStates[39] = 1;
        raw->emitReading(reading);
        QTRY_COMPARE(pressed.size(), 1);
        const QString control = pressed.at(0).at(0).toString();
        QVERIFY(ControlId::isDeviceButton(control));
        QCOMPARE(ControlId::label(control, ControlId::ControllerFamily::Generic),
                 QStringLiteral("Extra Button 40"));

        reading.buttonStates[39] = 0;
        raw->emitReading(reading);
        QTRY_COMPARE(released.size(), 1);
        QCOMPARE(released.at(0).at(0).toString(), control);
    }
};

QTEST_MAIN(GameInputRouterTest)
#include "tst_gameinputrouter.moc"
