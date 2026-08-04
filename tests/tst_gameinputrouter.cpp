#include "gameinput/FakeGameInputApi.h"
#include "gameinput/GameInputRouter.h"
#include "input/ControlId.h"
#include "storage/CaptureDatabase.h"

#include <QSignalSpy>
#include <QTemporaryDir>
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

    void standardReadingsStayShadowUntilCrossProviderDedup()
    {
        // Sony/XInput/WinMM still deliver standard controls through the
        // legacy arbitration and do not feed PhysicalControllerRegistry:
        // routing GameInput standard readings too would double-fire one
        // physical press. They must stay shadow-only until t25's real
        // cross-provider integration lands.
        auto api = std::make_unique<FakeGameInputApi>();
        auto* raw = api.get();
        GameInputRouter router(std::move(api));
        QSignalSpy pressed(&router, &GameInputRouter::systemControlPressed);
        QSignalSpy released(&router, &GameInputRouter::systemControlReleased);
        QVERIFY(router.start());
        auto reading = makeEvent(GameInputEventKind::Reading);
        reading.standardButtons = 0x00000004u; // GameInputGamepadA
        raw->emitReading(reading);
        QTRY_COMPARE(router.shadowReadingCount(), 1);
        reading.standardButtons = 0;
        raw->emitReading(reading);
        QTRY_COMPARE(router.shadowReadingCount(), 2);
        QCOMPARE(pressed.size(), 0);
        QCOMPARE(released.size(), 0);
    }

    void guideOnlyPadDoesNotReportShare()
    {
        auto api = std::make_unique<FakeGameInputApi>();
        auto* raw = api.get();
        GameInputRouter router(std::move(api));
        QVERIFY(router.start());
        auto added = makeEvent(GameInputEventKind::DeviceAdded);
        added.device.supportedSystemButtons = 0x1; // GameInputSystemButtonGuide only
        raw->emitDevice(added);
        QTRY_VERIFY(router.compatibilityReport()
                        .contains(QStringLiteral("Guide: Available")));
        QVERIFY(router.compatibilityReport()
                    .contains(QStringLiteral("Share: Not reported")));
    }

    void staleReadingAfterRemovalDoesNotResurrectDevice()
    {
        auto api = std::make_unique<FakeGameInputApi>();
        auto* raw = api.get();
        GameInputRouter router(std::move(api));
        QSignalSpy connected(&router, &GameInputRouter::deviceConnected);
        QSignalSpy disconnected(&router, &GameInputRouter::deviceDisconnected);
        QVERIFY(router.start());

        raw->emitDevice(makeEvent(GameInputEventKind::DeviceAdded));
        QTRY_COMPARE(connected.size(), 1);
        raw->emitDevice(makeEvent(GameInputEventKind::DeviceRemoved));
        QTRY_COMPARE(disconnected.size(), 1);

        // A reading that trails the removal (late callback) must not
        // re-register the device. The second pad is only a sentinel proving
        // the batch containing the stale reading was fully processed.
        raw->emitReading(makeEvent(GameInputEventKind::Reading));
        auto other = makeEvent(GameInputEventKind::DeviceAdded);
        other.deviceId = QStringLiteral("device-b");
        other.device.deviceId = other.deviceId;
        other.device.containerId = QStringLiteral("container-b");
        raw->emitDevice(other);
        QTRY_COMPARE(connected.size(), 2);
        QCOMPARE(router.shadowReadingCount(), 0);
        QVERIFY(router.registry()
                    .logicalIdFor(ControllerProvider::GameInput,
                                  QStringLiteral("device-a")).isEmpty());

        // A genuine re-add clears the tombstone and readings flow again.
        raw->emitDevice(makeEvent(GameInputEventKind::DeviceAdded));
        QTRY_COMPARE(connected.size(), 3);
        raw->emitReading(makeEvent(GameInputEventKind::Reading));
        QTRY_COMPARE(router.shadowReadingCount(), 1);
    }

    void changedLayoutReleasesHeldAndBlocksUntilConfirmed()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        CaptureDatabase db(dir.filePath(QStringLiteral("layout.db")));
        QVERIFY(db.open());
        auto api = std::make_unique<FakeGameInputApi>();
        auto* raw = api.get();
        GameInputRouter router(std::move(api), GameInputRouter::SupportMode::Auto, &db);
        QSignalSpy pressed(&router, &GameInputRouter::systemControlPressed);
        QSignalSpy released(&router, &GameInputRouter::systemControlReleased);
        QVERIFY(router.start());

        auto reading = makeEvent(GameInputEventKind::Reading);
        reading.device.extraButtonCount = 4;
        reading.buttonStates = {0, 0, 0, 0};
        raw->emitReading(reading);
        QTRY_COMPARE(router.shadowReadingCount(), 1);
        reading.buttonStates = {1, 0, 0, 0};
        raw->emitReading(reading);
        QTRY_COMPARE(pressed.size(), 1);
        const QString oldControl = pressed.at(0).at(0).toString();

        // Firmware/mode switch while the old control is held: the old layout
        // must be released and the new one must stay silent until the user
        // reconfirms it — an old index must never fire with a new meaning.
        reading.device.extraButtonCount = 3;
        reading.buttonStates = {1, 0, 0};
        raw->emitReading(reading);
        QTRY_COMPARE(released.size(), 1);
        QCOMPARE(released.at(0).at(0).toString(), oldControl);
        QVERIFY(router.layoutWarning());

        reading.buttonStates = {0, 1, 0};
        raw->emitReading(reading);
        QTRY_COMPARE(router.shadowReadingCount(), 4);
        QCOMPARE(pressed.size(), 1);
    }

    void disconnectReleasesHeldStateAndStrongReconnectRestoresIdentity()
    {
        auto api = std::make_unique<FakeGameInputApi>();
        auto* raw = api.get();
        GameInputRouter router(std::move(api));
        QSignalSpy connected(&router, &GameInputRouter::deviceConnected);
        QSignalSpy disconnected(&router, &GameInputRouter::deviceDisconnected);
        QSignalSpy reset(&router, &GameInputRouter::lifecycleReset);
        QSignalSpy pressed(&router, &GameInputRouter::systemControlPressed);
        QSignalSpy released(&router, &GameInputRouter::systemControlReleased);
        QVERIFY(router.start());

        raw->emitDevice(makeEvent(GameInputEventKind::DeviceAdded));
        QTRY_COMPARE(connected.size(), 1);
        const QString logicalId = connected.at(0).at(0).toString();
        QVERIFY(!connected.at(0).at(1).toBool());
        raw->emitSystem(makeEvent(GameInputEventKind::SystemButtonPressed, ControlId::Guide));
        QTRY_COMPARE(pressed.size(), 1);
        raw->emitDevice(makeEvent(GameInputEventKind::DeviceRemoved));
        QTRY_COMPARE(disconnected.size(), 1);
        QCOMPARE(reset.size(), 1);
        QCOMPARE(released.size(), 1);
        QVERIFY(!router.registry().controller(logicalId)->connected());

        raw->emitDevice(makeEvent(GameInputEventKind::DeviceAdded));
        QTRY_COMPARE(connected.size(), 2);
        QCOMPARE(connected.at(1).at(0).toString(), logicalId);
        QVERIFY(connected.at(1).at(1).toBool());
    }

    void sleepingOneOfTwoDevicesReleasesOnlyThatDevice()
    {
        auto api = std::make_unique<FakeGameInputApi>();
        auto* raw = api.get();
        GameInputRouter router(std::move(api));
        QSignalSpy pressed(&router, &GameInputRouter::systemControlPressed);
        QSignalSpy released(&router, &GameInputRouter::systemControlReleased);
        QVERIFY(router.start());
        auto first = makeEvent(GameInputEventKind::SystemButtonPressed, ControlId::Guide);
        auto second = first;
        second.deviceId = QStringLiteral("device-b");
        second.device.deviceId = second.deviceId;
        second.device.containerId = QStringLiteral("container-b");
        raw->emitSystem(first);
        raw->emitSystem(second);
        QTRY_COMPARE(pressed.size(), 2);

        first.kind = GameInputEventKind::Sleep;
        raw->emitDevice(first);
        QTRY_COMPARE(released.size(), 1);
        QCOMPARE(released.at(0).at(1).toString(),
                 router.registry().logicalIdFor(ControllerProvider::GameInput,
                                                QStringLiteral("device-a")));
    }

    void compatibilityReportIsUsefulAndRedacted()
    {
        auto api = std::make_unique<FakeGameInputApi>();
        auto* raw = api.get();
        GameInputRouter router(std::move(api));
        QVERIFY(router.start());
        raw->emitDevice(makeEvent(GameInputEventKind::DeviceAdded));
        QTRY_VERIFY(router.controllerSummary().contains(QStringLiteral("Modern test pad")));
        const QString report = router.compatibilityReport();
        QVERIFY(report.contains(QStringLiteral("Anonymous ID:")));
        QVERIFY(report.contains(QStringLiteral("Providers: GameInput")));
        QVERIFY(report.contains(QStringLiteral("Share: Available")));
        QVERIFY(!report.contains(QStringLiteral("container-a")));
        QVERIFY(!report.contains(QStringLiteral("device-a")));
    }
};

QTEST_MAIN(GameInputRouterTest)
#include "tst_gameinputrouter.moc"
