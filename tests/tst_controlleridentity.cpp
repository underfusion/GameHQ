#include "input/ControllerIdentity.h"

#include <QtTest>

// The correlation is allowed to say "I don't know" — a wrong stable identity
// would silently attach one pad's bindings to another, which is the exact bug
// the slot fingerprint already has.
class ControllerIdentityTest : public QObject
{
    Q_OBJECT

private slots:
    void oneDeviceOneSlotIsUnambiguous()
    {
        QCOMPARE(ControllerIdentity::resolveXInputFingerprint(
                     {QStringLiteral("3537:1004")}, 1, 0),
                 QStringLiteral("3537:1004"));
        // The same device exposing several IG_ collections is still one device.
        QCOMPARE(ControllerIdentity::resolveXInputFingerprint(
                     {QStringLiteral("3537:1004"), QStringLiteral("3537:1004")}, 1, 2),
                 QStringLiteral("3537:1004"));
    }

    void anyAmbiguityKeepsTheHonestSlotKey()
    {
        // Nothing visible (HidHide cloak, no IG_ collections).
        QCOMPARE(ControllerIdentity::resolveXInputFingerprint({}, 1, 0),
                 QStringLiteral("xinput.slot0"));
        // Two distinct devices: which one is in which slot is unknowable here.
        QCOMPARE(ControllerIdentity::resolveXInputFingerprint(
                     {QStringLiteral("3537:1004"), QStringLiteral("045E:02FF")}, 1, 1),
                 QStringLiteral("xinput.slot1"));
        // One device but two connected slots: same problem.
        QCOMPARE(ControllerIdentity::resolveXInputFingerprint(
                     {QStringLiteral("3537:1004")}, 2, 0),
                 QStringLiteral("xinput.slot0"));
        // Empty identity strings carry no information.
        QCOMPARE(ControllerIdentity::resolveXInputFingerprint({QString()}, 1, 3),
                 QStringLiteral("xinput.slot3"));
    }

    void legacyFingerprintRoundTrip()
    {
        QCOMPARE(ControllerIdentity::legacySlotFingerprint(2),
                 QStringLiteral("xinput.slot2"));
        QVERIFY(ControllerIdentity::isLegacySlotFingerprint(QStringLiteral("xinput.slot0")));
        QVERIFY(!ControllerIdentity::isLegacySlotFingerprint(QStringLiteral("3537:1004")));
        QVERIFY(!ControllerIdentity::isLegacySlotFingerprint({}));
    }
};

QTEST_APPLESS_MAIN(ControllerIdentityTest)
#include "tst_controlleridentity.moc"
