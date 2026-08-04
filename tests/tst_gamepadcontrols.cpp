#include "input/Gamepad.h"

#include <QSignalSpy>
#include <QtTest>

class PublishingGamepad final : public Gamepad
{
public:
    using Gamepad::publishButtonPressed;
    using Gamepad::publishButtonReleased;
    using Gamepad::publishControlPressed;
    using Gamepad::publishControlReleased;

    bool start() override { return true; }

    ControlId::DeviceProfile profile() const override
    {
        ControlId::DeviceProfile result;
        result.family = ControlId::ControllerFamily::Generic;
        result.backend = QStringLiteral("gameinput");
        result.fingerprint = QStringLiteral("logical-device-1");
        result.displayName = QStringLiteral("Test controller");
        return result;
    }
};

class GamepadControlsTest : public QObject
{
    Q_OBJECT

private slots:
    void directControlIdsAreNotLimitedTo32Buttons()
    {
        PublishingGamepad pad;
        QSignalSpy pressed(&pad, &Gamepad::controlPressed);
        QSignalSpy released(&pad, &Gamepad::controlReleased);
        const QString control = QStringLiteral("device:logical-device-1:button:47");

        pad.publishControlPressed(control);
        pad.publishControlReleased(control);

        QCOMPARE(pressed.size(), 1);
        QCOMPARE(released.size(), 1);
        QCOMPARE(pressed.at(0).at(0).toString(), control);
        QCOMPARE(released.at(0).at(0).toString(), control);
        QCOMPARE(pressed.at(0).at(2).toString(), QStringLiteral("gameinput"));
    }

    void integerHelpersRemainAdapters()
    {
        PublishingGamepad pad;
        QSignalSpy pressed(&pad, &Gamepad::controlPressed);

        pad.publishButtonPressed(Gamepad::Cross);
        pad.publishButtonPressed(Gamepad::GenericButtonBase + 4);

        QCOMPARE(pressed.size(), 2);
        QCOMPARE(pressed.at(0).at(0).toString(), ControlId::FaceSouth);
        QCOMPARE(pressed.at(1).at(0).toString(), ControlId::genericButton(4));
    }

    void invalidLegacyIndexPublishesNothing()
    {
        PublishingGamepad pad;
        QSignalSpy pressed(&pad, &Gamepad::controlPressed);
        pad.publishButtonPressed(-1);
        QCOMPARE(pressed.size(), 0);
    }
};

QTEST_MAIN(GamepadControlsTest)
#include "tst_gamepadcontrols.moc"
