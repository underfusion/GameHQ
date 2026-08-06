#include "input/SelectiveRawHidFallback.h"
#include "input/ControlId.h"

#include <QtTest>

using namespace ModernInput;

class SelectiveRawHidFallbackTest : public QObject
{
    Q_OBJECT

private slots:
    void cleanup()
    {
        auto& fallback = SelectiveRawHidFallback::instance();
        fallback.endProbe();
        fallback.setBoundControls({});
    }

    void dormantUntilProbeOrExplicitBinding()
    {
        auto& fallback = SelectiveRawHidFallback::instance();
        QVERIFY(!fallback.shouldObserve(QStringLiteral("container-secret"), 0x0c, 0x00b5));
        QVERIFY(fallback.observeUsage(QStringLiteral("container-secret"), 0x0c, 0x00b5, true)
                    .isEmpty());

        const QString bound = ControlId::rawHidUsage(
            QStringLiteral("container-secret"), 0x0c, 0x00b5);
        fallback.setBoundControls({bound});
        QVERIFY(fallback.shouldObserve(QStringLiteral("container-secret"), 0x0c, 0x00b5));
        QCOMPARE(fallback.observeUsage(QStringLiteral("container-secret"), 0x0c, 0x00b5,
                                       true), bound);
        QVERIFY(!fallback.shouldObserve(QStringLiteral("container-secret"), 0x0c, 0x00b6));
        QVERIFY(ControlId::isCanonical(bound));
        QVERIFY(!bound.contains(QStringLiteral("container-secret")));
    }

    void probeReportsRawHidAndKeyboardMacros()
    {
        auto& fallback = SelectiveRawHidFallback::instance();
        fallback.beginProbe();
        const QString control = fallback.observeUsage(
            QStringLiteral("device"), 0x0c, 0x00b5, true);
        QVERIFY(ControlId::isRawHidUsage(control));
        fallback.observeKeyboard(QStringLiteral("Print Screen"));
        const QString summary = fallback.probeSummary();
        QVERIFY(summary.contains(QStringLiteral("Raw HID")));
        QVERIFY(summary.contains(QStringLiteral("Keyboard macro: Print Screen")));
    }

    void keyboardOnlyProbeSteersTowardAKeyboardBinding()
    {
        // A GameSir-style Share button whose only trace is a keyboard macro:
        // the summary must say it is not controller input and point at the
        // Keyboard section (with the remap-to-F13 escape hatch).
        auto& fallback = SelectiveRawHidFallback::instance();
        fallback.beginProbe();
        fallback.observeKeyboard(QStringLiteral("Print Screen"));
        const QString summary = fallback.probeSummary();
        QVERIFY(summary.contains(QStringLiteral("Keyboard macro: Print Screen")));
        QVERIFY(summary.contains(QStringLiteral("keyboard shortcut rather than")));
        QVERIFY(summary.contains(QStringLiteral("F13")));
    }

    void emptyProbeIsHonest()
    {
        auto& fallback = SelectiveRawHidFallback::instance();
        fallback.beginProbe();
        const QString summary = fallback.probeSummary();
        QVERIFY(summary.contains(QStringLiteral("No event was received")));
        QVERIFY(summary.contains(QStringLiteral("firmware mode may disable")));
        QVERIFY(summary.contains(QStringLiteral("another controller mode")));
    }
};

QTEST_MAIN(SelectiveRawHidFallbackTest)
#include "tst_selectiverawhid.moc"
