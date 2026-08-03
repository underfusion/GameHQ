// TriggerSpec / GestureSpec: serialization contract and the validation that
// guards the parse boundary. Three things must hold for the model to be safe to
// build on: every value written before patterns existed still parses to exactly
// what it meant, every valid spec survives serialize -> parse unchanged, and
// anything this build cannot understand is rejected instead of approximated.

#include "input/BindingPattern.h"
#include "input/ControlId.h"

#include <QTest>

using Gesture = GestureSpec::Kind;
using Trigger = TriggerSpec::Kind;

class BindingPatternTest : public QObject
{
    Q_OBJECT

private slots:
    // ------------------------------------------------------------- triggers

    void legacySingleTriggersParseUnchanged_data()
    {
        QTest::addColumn<QString>("code");
        QTest::newRow("controller face") << ControlId::FaceSouth;
        QTest::newRow("controller share") << ControlId::Capture;
        QTest::newRow("generic button") << ControlId::genericButton(7);
        QTest::newRow("keyboard chord") << QStringLiteral("Ctrl+Shift+S");
        QTest::newRow("keyboard key") << QStringLiteral("PgDown");
        QTest::newRow("unknown code") << QStringLiteral("something.else");
    }

    void legacySingleTriggersParseUnchanged()
    {
        QFETCH(QString, code);
        const auto parsed = TriggerSpec::parse(code);
        QVERIFY2(parsed.ok, qPrintable(parsed.error));
        QCOMPARE(parsed.trigger.kind, Trigger::Single);
        QCOMPARE(parsed.trigger.controls.size(), qsizetype(1));
        QCOMPARE(parsed.trigger.firstControl(), code);
        // The stored value is byte-identical, so nothing on disk is rewritten.
        QCOMPARE(parsed.trigger.serialize(), code);
    }

    void chordSerializationRoundTrips()
    {
        const TriggerSpec chord = TriggerSpec::orderedChord(ControlId::Capture, ControlId::Guide);
        QCOMPARE(chord.serialize(),
                 QStringLiteral("chord:v1:gamepad.capture>gamepad.guide"));

        const auto parsed = TriggerSpec::parse(chord.serialize());
        QVERIFY2(parsed.ok, qPrintable(parsed.error));
        QCOMPARE(parsed.trigger, chord);
        QCOMPARE(parsed.trigger.kind, Trigger::OrderedChord);
        QCOMPARE(parsed.trigger.firstControl(), ControlId::Capture);
        QCOMPARE(parsed.trigger.secondControl(), ControlId::Guide);
        QVERIFY(parsed.trigger.isChord());
    }

    void chordOrderIsSignificant()
    {
        const TriggerSpec forward = TriggerSpec::orderedChord(ControlId::Capture, ControlId::Guide);
        const TriggerSpec reverse = TriggerSpec::orderedChord(ControlId::Guide, ControlId::Capture);
        QVERIFY(forward.serialize() != reverse.serialize());
        QVERIFY(!(forward == reverse));
    }

    void chordOverGenericButtonsIsAccepted()
    {
        const TriggerSpec chord = TriggerSpec::orderedChord(ControlId::genericButton(4),
                                                            ControlId::genericButton(9));
        const auto parsed = TriggerSpec::parse(chord.serialize());
        QVERIFY2(parsed.ok, qPrintable(parsed.error));
        QCOMPARE(parsed.trigger, chord);
    }

    void invalidTriggersAreRejected_data()
    {
        QTest::addColumn<QString>("code");
        QTest::newRow("empty") << QString();
        QTest::newRow("unknown version") << QStringLiteral("chord:v2:gamepad.capture>gamepad.guide");
        QTest::newRow("bare namespace") << QStringLiteral("chord:");
        QTest::newRow("no payload") << QStringLiteral("chord:v1:");
        QTest::newRow("one control") << QStringLiteral("chord:v1:gamepad.capture");
        QTest::newRow("three controls")
            << QStringLiteral("chord:v1:gamepad.capture>gamepad.guide>gamepad.menu");
        QTest::newRow("same control twice")
            << QStringLiteral("chord:v1:gamepad.capture>gamepad.capture");
        QTest::newRow("empty first") << QStringLiteral("chord:v1:>gamepad.guide");
        QTest::newRow("empty second") << QStringLiteral("chord:v1:gamepad.capture>");
        QTest::newRow("non-canonical control")
            << QStringLiteral("chord:v1:gamepad.capture>Ctrl+Shift+S");
        QTest::newRow("typo control")
            << QStringLiteral("chord:v1:gamepad.capture>gamepad.face_souht");
    }

    void invalidTriggersAreRejected()
    {
        QFETCH(QString, code);
        const auto parsed = TriggerSpec::parse(code);
        QVERIFY2(!parsed.ok, qPrintable(QStringLiteral("'%1' should not parse").arg(code)));
        QVERIFY(!parsed.error.isEmpty());
    }

    void reservedNamespaceCannotBecomeASingleControl()
    {
        // Otherwise a control id starting with "chord:" would serialize into a
        // string this parser reads back as a chord.
        QString error;
        QVERIFY(!TriggerSpec::single(QStringLiteral("chord:v1:a>b")).isValid(&error));
        QVERIFY(!error.isEmpty());
    }

    // ------------------------------------------------------------- gestures

    void legacyActivationsParse_data()
    {
        QTest::addColumn<QString>("activation");
        QTest::addColumn<int>("holdMs");
        QTest::addColumn<int>("kind");
        QTest::addColumn<int>("tapCount");

        QTest::newRow("empty means press") << QString() << 0 << int(Gesture::Press) << 1;
        QTest::newRow("press") << QStringLiteral("press") << 0 << int(Gesture::Press) << 1;
        QTest::newRow("tap") << QStringLiteral("tap") << 0 << int(Gesture::Tap) << 1;
        QTest::newRow("double_tap") << QStringLiteral("double_tap") << 0 << int(Gesture::Tap) << 2;
        QTest::newRow("hold") << QStringLiteral("hold") << 2000 << int(Gesture::Hold) << 1;
        QTest::newRow("hold default duration")
            << QStringLiteral("hold") << 0 << int(Gesture::Hold) << 1;
    }

    void legacyActivationsParse()
    {
        QFETCH(QString, activation);
        QFETCH(int, holdMs);
        QFETCH(int, kind);
        QFETCH(int, tapCount);

        const auto parsed = GestureSpec::parse(activation, 1, holdMs);
        QVERIFY2(parsed.ok, qPrintable(parsed.error));
        QCOMPARE(int(parsed.gesture.kind), kind);
        QCOMPARE(parsed.gesture.tapCount, tapCount);
        QCOMPARE(parsed.gesture.holdMs, holdMs);
    }

    void tapCountsRoundTrip()
    {
        for (int count = 1; count <= GestureSpec::kMaxTapCount; ++count) {
            const GestureSpec spec = GestureSpec::tap(count);
            QVERIFY(spec.isValid());
            const auto parsed = GestureSpec::parse(spec.activationCode(), spec.tapCount,
                                                   spec.holdMs);
            QVERIFY2(parsed.ok, qPrintable(parsed.error));
            QCOMPARE(parsed.gesture, spec);
        }
    }

    void doubleTapKeepsItsLegacySpellingButTripleTapDoesNot()
    {
        QVERIFY(GestureSpec::tap(2).hasLegacyActivation());
        QCOMPARE(GestureSpec::tap(2).legacyActivationCode(), QStringLiteral("double_tap"));
        QCOMPARE(GestureSpec::tap(2).activationCode(), QStringLiteral("tap"));

        // Nothing may quietly write a triple tap as a double tap: the caller has
        // to see that persisting it needs the tap-count column.
        QVERIFY(!GestureSpec::tap(3).hasLegacyActivation());

        QVERIFY(GestureSpec::press().hasLegacyActivation());
        QCOMPARE(GestureSpec::press().legacyActivationCode(), QStringLiteral("press"));
        QCOMPARE(GestureSpec::hold(1500).legacyActivationCode(), QStringLiteral("hold"));
    }

    void doubleTapRowKeepsCountTwo()
    {
        // A schema without the column reports 1 = unset; an explicit 2 agrees.
        QCOMPARE(GestureSpec::parse(QStringLiteral("double_tap"), 1, 0).gesture.tapCount, 2);
        QCOMPARE(GestureSpec::parse(QStringLiteral("double_tap"), 2, 0).gesture.tapCount, 2);
        // A contradiction is a corrupted row, not something to reinterpret.
        QVERIFY(!GestureSpec::parse(QStringLiteral("double_tap"), 3, 0).ok);
    }

    void invalidGesturesAreRejected_data()
    {
        QTest::addColumn<QString>("activation");
        QTest::addColumn<int>("tapCount");
        QTest::addColumn<int>("holdMs");

        QTest::newRow("unknown activation") << QStringLiteral("triple_tap") << 1 << 0;
        QTest::newRow("garbage activation") << QStringLiteral("hodl") << 1 << 0;
        QTest::newRow("press with hold") << QStringLiteral("press") << 1 << 500;
        QTest::newRow("press with tap count") << QStringLiteral("press") << 2 << 0;
        QTest::newRow("tap with hold") << QStringLiteral("tap") << 1 << 500;
        QTest::newRow("tap count zero") << QStringLiteral("tap") << 0 << 0;
        QTest::newRow("tap count negative") << QStringLiteral("tap") << -1 << 0;
        QTest::newRow("tap count above max") << QStringLiteral("tap") << 4 << 0;
        QTest::newRow("hold with tap count") << QStringLiteral("hold") << 2 << 1000;
        QTest::newRow("hold negative") << QStringLiteral("hold") << 1 << -1;
    }

    void invalidGesturesAreRejected()
    {
        QFETCH(QString, activation);
        QFETCH(int, tapCount);
        QFETCH(int, holdMs);
        const auto parsed = GestureSpec::parse(activation, tapCount, holdMs);
        QVERIFY(!parsed.ok);
        QVERIFY(!parsed.error.isEmpty());
    }

    // ------------------------------------------------------ combined pattern

    void legacyControllerRowsStillParse_data()
    {
        QTest::addColumn<QString>("trigger");
        QTest::addColumn<QString>("activation");
        QTest::addColumn<int>("holdMs");

        // Exactly the shapes the shipped defaults use today.
        QTest::newRow("screenshot tap") << ControlId::Capture << QStringLiteral("tap") << 0;
        QTest::newRow("save replay hold") << ControlId::Capture << QStringLiteral("hold") << 2000;
        QTest::newRow("overlay press") << ControlId::Guide << QStringLiteral("press") << 0;
        QTest::newRow("overlay double tap")
            << ControlId::Capture << QStringLiteral("double_tap") << 0;
        QTest::newRow("bulk hold") << ControlId::FaceSouth << QStringLiteral("hold") << 1000;
    }

    void legacyControllerRowsStillParse()
    {
        QFETCH(QString, trigger);
        QFETCH(QString, activation);
        QFETCH(int, holdMs);
        const auto parsed = BindingPattern::parse(QStringLiteral("controller"), trigger,
                                                  activation, 1, holdMs);
        QVERIFY2(parsed.ok, qPrintable(parsed.error));
        QCOMPARE(parsed.pattern.trigger.firstControl(), trigger);
        QCOMPARE(parsed.pattern.gesture.holdMs, holdMs);
    }

    void keyboardRowsStillParse()
    {
        const auto parsed = BindingPattern::parse(QStringLiteral("keyboard"),
                                                  QStringLiteral("Ctrl+Shift+G"),
                                                  QStringLiteral("press"), 1, 0);
        QVERIFY2(parsed.ok, qPrintable(parsed.error));
        QCOMPARE(parsed.pattern.trigger.kind, Trigger::Single);
        QCOMPARE(parsed.pattern.gesture.kind, Gesture::Press);
    }

    void chordIsControllerOnlyAndPressOnly()
    {
        const QString code = TriggerSpec::orderedChord(ControlId::Capture, ControlId::Guide)
                                 .serialize();

        const auto controller = BindingPattern::parse(QStringLiteral("controller"), code,
                                                      QStringLiteral("press"), 1, 0);
        QVERIFY2(controller.ok, qPrintable(controller.error));
        QVERIFY(controller.pattern.trigger.isChord());

        // Version 1: no chord + tap, no chord + hold, no chord on a keyboard.
        QVERIFY(!BindingPattern::parse(QStringLiteral("controller"), code,
                                       QStringLiteral("tap"), 1, 0).ok);
        QVERIFY(!BindingPattern::parse(QStringLiteral("controller"), code,
                                       QStringLiteral("hold"), 1, 800).ok);
        QVERIFY(!BindingPattern::parse(QStringLiteral("keyboard"), code,
                                       QStringLiteral("press"), 1, 0).ok);
        QVERIFY(!BindingPattern::parse(QStringLiteral("mouse"), code,
                                       QStringLiteral("press"), 1, 0).ok);
    }

    void unknownChordVersionNeverExecutes()
    {
        // The whole point of the reserved namespace: a build that does not know
        // v2 refuses it rather than falling back to "some button".
        const auto parsed = BindingPattern::parse(QStringLiteral("controller"),
                                                  QStringLiteral("chord:v2:gamepad.capture>gamepad.guide"),
                                                  QStringLiteral("press"), 1, 0);
        QVERIFY(!parsed.ok);
        QVERIFY(!parsed.error.isEmpty());
        QVERIFY(parsed.pattern.trigger.controls.isEmpty());
    }

    void patternRoundTripsThroughSerialization()
    {
        const QVector<BindingPattern> patterns{
            {TriggerSpec::single(ControlId::Capture), GestureSpec::press()},
            {TriggerSpec::single(ControlId::Capture), GestureSpec::tap(1)},
            {TriggerSpec::single(ControlId::Capture), GestureSpec::tap(2)},
            {TriggerSpec::single(ControlId::Capture), GestureSpec::tap(3)},
            {TriggerSpec::single(ControlId::Capture), GestureSpec::hold(2000)},
            {TriggerSpec::single(ControlId::Capture), GestureSpec::hold(0)},
            {TriggerSpec::single(QStringLiteral("Ctrl+Shift+S")), GestureSpec::press()},
            {TriggerSpec::orderedChord(ControlId::Capture, ControlId::Guide), GestureSpec::press()},
            {TriggerSpec::orderedChord(ControlId::Menu, ControlId::genericButton(3)),
             GestureSpec::press()},
        };

        for (const BindingPattern& pattern : patterns) {
            const QString group = pattern.trigger.firstControl().startsWith(QLatin1String("Ctrl"))
                                      ? QStringLiteral("keyboard")
                                      : QStringLiteral("controller");
            QVERIFY(pattern.isValid(group));
            const auto parsed = BindingPattern::parse(group, pattern.trigger.serialize(),
                                                      pattern.gesture.activationCode(),
                                                      pattern.gesture.tapCount,
                                                      pattern.gesture.holdMs);
            QVERIFY2(parsed.ok, qPrintable(parsed.error));
            QCOMPARE(parsed.pattern, pattern);
        }
    }

    void labelsShowBothChordControls()
    {
        const TriggerSpec chord = TriggerSpec::orderedChord(ControlId::Capture, ControlId::Guide);
        QCOMPARE(chord.label(ControlId::ControllerFamily::PlayStation),
                 QStringLiteral("Share + PS"));
        QCOMPARE(chord.label(ControlId::ControllerFamily::Xbox), QStringLiteral("View + Guide"));
        QCOMPARE(TriggerSpec::single(ControlId::Capture)
                     .label(ControlId::ControllerFamily::PlayStation),
                 QStringLiteral("Share"));
    }

    void canonicalControlsAreRecognized()
    {
        QVERIFY(ControlId::isCanonical(ControlId::FaceSouth));
        QVERIFY(ControlId::isCanonical(ControlId::Guide));
        QVERIFY(ControlId::isCanonical(ControlId::genericButton(0)));
        QVERIFY(ControlId::isCanonical(ControlId::genericButton(31)));
        QVERIFY(!ControlId::isCanonical(QStringLiteral("gamepad.button.x")));
        QVERIFY(!ControlId::isCanonical(QStringLiteral("Ctrl+Shift+S")));
        QVERIFY(!ControlId::isCanonical(QString()));
    }
};

QTEST_MAIN(BindingPatternTest)
#include "tst_bindingpattern.moc"
