#include <QtTest>

#include "input/ControllerArbitration.h"

class ControllerArbitrationTest : public QObject
{
    Q_OBJECT

private slots:
    void backendWithoutActivityYieldsImmediately()
    {
        QVERIFY(ControllerArbitration::backendMayTakeOver(false, true, 0, 1));
    }

    void mirroredBackendPressIsIgnored()
    {
        QVERIFY(!ControllerArbitration::backendMayTakeOver(
            true, true, 1000, 1100));
    }

    void activeBackendYieldsAfterDuplicateWindow()
    {
        QVERIFY(ControllerArbitration::backendMayTakeOver(
            true, true, 1000, 1101));
    }

    void differentControlTakesOverImmediately()
    {
        QVERIFY(ControllerArbitration::backendMayTakeOver(
            true, false, 1000, 1001));
    }

    void physicalSonyPadWinsImmediately()
    {
        QVERIFY(ControllerArbitration::sonyDeviceMayTakeOver(
            true, 3, 1, 20, 19, 1000));
    }

    void lowerPrioritySonyPathWaitsForRealIdle()
    {
        QVERIFY(!ControllerArbitration::sonyDeviceMayTakeOver(
            true, 1, 3, 1100, 100, 1000));
        QVERIFY(ControllerArbitration::sonyDeviceMayTakeOver(
            true, 1, 3, 1101, 100, 1000));
    }

    void idleTrafficCannotStealActiveSonyPad()
    {
        QVERIFY(!ControllerArbitration::sonyDeviceMayTakeOver(
            false, 3, 1, 5000, 0, 1000));
    }
};

QTEST_APPLESS_MAIN(ControllerArbitrationTest)
#include "tst_controllerarbitration.moc"
