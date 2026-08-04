#include "input/XInputDevice.h"

#include <QSignalSpy>
#include <QtTest>

class TestXInputDevice final : public XInputDevice
{
public:
    using XInputDevice::setSlotState;
};

class XInputIdentityTest : public QObject
{
    Q_OBJECT

private slots:
    void everyEdgeCarriesItsActualSlotIdentity()
    {
        TestXInputDevice pad;
        QSignalSpy pressed(&pad, &Gamepad::controlPressed);
        QSignalSpy released(&pad, &Gamepad::controlReleased);

        pad.setSlotState(0, 0, true);
        pad.setSlotState(1, 0, true);
        pad.setSlotState(1, 1u << Gamepad::Cross, true);
        pad.setSlotState(1, 0, true);

        QCOMPARE(pressed.size(), 1);
        QCOMPARE(released.size(), 1);
        QCOMPARE(pressed.at(0).at(0).toString(), ControlId::FaceSouth);
        QCOMPARE(pressed.at(0).at(3).toString(), QStringLiteral("xinput.slot1"));
        QCOMPARE(released.at(0).at(3).toString(), QStringLiteral("xinput.slot1"));
        QVERIFY(pressed.at(0).at(3).toString() != QStringLiteral("xinput.slot0"));
    }

    void endpointIdentityDoesNotCollapseTwoSlots()
    {
        TestXInputDevice pad;
        pad.setKnownDeviceIdentity(0, QStringLiteral("hid.endpoint.pad-a"),
                                   QStringLiteral("045e:0b12"));
        pad.setKnownDeviceIdentity(1, QStringLiteral("hid.endpoint.pad-b"),
                                   QStringLiteral("045e:0b12"));
        QSignalSpy pressed(&pad, &Gamepad::controlPressed);

        pad.setSlotState(0, 0, true);
        pad.setSlotState(1, 0, true);
        pad.setSlotState(0, 1u << Gamepad::Cross, true);
        pad.setSlotState(1, 1u << Gamepad::Circle, true);

        QCOMPARE(pressed.size(), 2);
        QCOMPARE(pressed.at(0).at(3).toString(), QStringLiteral("xinput.slot0"));
        QCOMPARE(pressed.at(1).at(3).toString(), QStringLiteral("xinput.slot1"));
        QCOMPARE(pad.profileForSlot(0).endpointId, QStringLiteral("hid.endpoint.pad-a"));
        QCOMPARE(pad.profileForSlot(1).endpointId, QStringLiteral("hid.endpoint.pad-b"));
    }
};

QTEST_MAIN(XInputIdentityTest)
#include "tst_xinputidentity.moc"
