#include "capture/HdrCapabilities.h"
#include "config/ConfigKeys.h"

#include <QTest>

#include <algorithm>

class TestHdrCapabilities : public QObject
{
    Q_OBJECT

private slots:
    void reportsCurrentDisplayState()
    {
        const capture::HdrReport report = capture::HdrCapabilities::query(
            ConfigKeys::InternalCaptureExperimentalHdrDefault);
        for (const QString& line : report.summaryLines())
            qInfo().noquote() << line;
        if (report.outputs.isEmpty())
            QSKIP("No local DXGI display outputs are available");
        QVERIFY(std::any_of(report.outputs.cbegin(), report.outputs.cend(),
                            [](const capture::HdrOutputInfo& output) {
                                return output.valid;
                            }));
    }

    void publicDefaultDescribesToneMappedOutputs()
    {
        QVERIFY(ConfigKeys::InternalCaptureExperimentalHdrDefault);
        const capture::HdrReport report = capture::HdrCapabilities::query(
            ConfigKeys::InternalCaptureExperimentalHdrDefault);
        QVERIFY(report.captureFormat.contains(QStringLiteral("FP16")));
        QVERIFY(report.screenshotSupport.contains(QStringLiteral("Tone-mapped")));
        QVERIFY(report.activeFallback.contains(QStringLiteral("tone-mapped"),
                                               Qt::CaseInsensitive)
                || !report.anyHdrActive);
    }

    void comparesRuntimeDisplayState()
    {
        capture::HdrOutputInfo output;
        output.deviceName = QStringLiteral("\\\\.\\DISPLAY1");
        output.desktopRect = QRect(0, 0, 3840, 2160);
        output.valid = true;
        output.hdrActive = true;
        output.advancedColorMode = 2;
        output.bitsPerColor = 10;
        output.sdrWhiteLevelNits = 240.0f;

        capture::HdrReport original;
        original.outputs.push_back(output);
        original.anyHdrActive = true;
        original.hevcMain10Encoder = true;

        capture::HdrReport same = original;
        same.hevcMain10Encoder = false;
        same.captureFormat = QStringLiteral("Different configured policy");
        QVERIFY(capture::HdrCapabilities::sameDisplayState(original, same));

        capture::HdrReport toggled = original;
        toggled.outputs[0].hdrActive = false;
        toggled.outputs[0].advancedColorMode = 0;
        toggled.anyHdrActive = false;
        QVERIFY(!capture::HdrCapabilities::sameDisplayState(original, toggled));

        capture::HdrReport whiteChanged = original;
        whiteChanged.outputs[0].sdrWhiteLevelNits = 480.0f;
        QVERIFY(!capture::HdrCapabilities::sameDisplayState(original, whiteChanged));
    }
};

QTEST_GUILESS_MAIN(TestHdrCapabilities)
#include "tst_hdrcapabilities.moc"
