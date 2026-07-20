#include "capture/hdr/HdrToneMapMath.h"

#include <QTest>

#include <cmath>
#include <limits>

using capture::hdr::shouldAttemptFp16Capture;
using capture::hdr::toneMapPixel;

// Pure-logic CPU oracle for the tone-map math shared with GpuToneMapper's
// embedded HLSL (t24), plus the pure decision logic FramePumpService::
// createSession gates the whole experimental path on. No D3D/GPU dependency,
// so this always runs in CI — the GPU-side smoke test lives separately and
// self-skips when unsupported.
class TestHdrToneMap : public QObject
{
    Q_OBJECT

private slots:
    void black_staysBlack()
    {
        const auto px = toneMapPixel(0.0f, 0.0f, 0.0f, 0.0f);
        QCOMPARE(int(px.r), 0);
        QCOMPARE(int(px.g), 0);
        QCOMPARE(int(px.b), 0);
        QCOMPARE(int(px.a), 0);
    }

    // 18% gray (0.18 linear) sits below the knee (0.9), so it is pure
    // identity + the standard sRGB OETF — a well-known reference point
    // (~46% in 8-bit sRGB) independent of this curve's shoulder shape.
    void eighteenPercentGray_matchesStandardSrgbEncode()
    {
        const auto px = toneMapPixel(0.18f, 0.18f, 0.18f);
        QVERIFY2(px.r >= 115 && px.r <= 122,
                 qPrintable(QStringLiteral("got %1, expected ~118 (18%% gray sRGB-encoded)").arg(px.r)));
    }

    // SDR white (1.0 scRGB) sits just above the knee, so it is intentionally
    // NOT quite full 255 — a few percent of headroom is reserved so
    // highlights above white have somewhere to compress to instead of
    // clipping instantly. It must still read as "essentially white."
    void sdrWhite_isNearWhiteNotExact()
    {
        const auto px = toneMapPixel(1.0f, 1.0f, 1.0f, 1.0f);
        QVERIFY2(px.r > 235 && px.r < 255,
                 qPrintable(QStringLiteral("got %1, expected near-but-not-255").arg(px.r)));
        QCOMPARE(int(px.a), 255);
    }

    // Values in [0, kneeStart] must not brighten or darken relative to each
    // other — identity + sRGB-encode is monotonic.
    void ramp_isMonotonicBelowKnee()
    {
        int prev = -1;
        for (float v = 0.0f; v <= 0.9f; v += 0.1f) {
            const auto px = toneMapPixel(v, v, v);
            QVERIFY(int(px.r) >= prev);
            prev = int(px.r);
        }
    }

    // The whole point of a shoulder curve over a hard clip: highlights well
    // above reference white must remain DISTINGUISHABLE from each other,
    // strictly increasing toward (but never reaching) white — not a single
    // flat clipped plateau. This is exactly what GPT-5's review flagged the
    // hard-clip version for failing.
    void highlights_areDistinctAndMonotonic()
    {
        const float values[] = { 1.0f, 1.5f, 2.0f, 4.0f, 8.0f, 16.0f };
        int prev = -1;
        for (float v : values) {
            const auto px = toneMapPixel(v, v, v);
            QVERIFY2(int(px.r) > prev,
                     qPrintable(QStringLiteral("value %1 -> %2 not strictly greater than previous %3")
                                    .arg(v).arg(px.r).arg(prev)));
            QVERIFY2(px.r < 255,
                     qPrintable(QStringLiteral("value %1 -> %2 clipped to full white").arg(v).arg(px.r)));
            prev = int(px.r);
        }
    }

    // Extreme/degenerate input must still produce a finite, valid, clamped
    // 8-bit result — never NaN, never wrap, never crash.
    void extremeHighlight_staysFiniteAndClamped()
    {
        for (float v : { 1e6f, 1e10f, std::numeric_limits<float>::max() / 2.0f }) {
            const auto px = toneMapPixel(v, v, v);
            QVERIFY(px.r <= 255);
            QVERIFY(px.g <= 255);
            QVERIFY(px.b <= 255);
        }
    }

    // Wide-gamut BT.2020 colors outside sRGB primaries can arrive as negative
    // scRGB components — must clip to 0, never underflow/wrap unsigned char.
    void negativeInput_clipsToZero()
    {
        const auto px = toneMapPixel(-0.5f, -100.0f, 0.5f);
        QCOMPARE(int(px.r), 0);
        QCOMPARE(int(px.g), 0);
        QVERIFY(px.b > 0);
    }

    // Alpha is a plain linear-to-8-bit pass-through, not tone-mapped —
    // capture alpha is always opaque coverage, never HDR luminance.
    void alpha_isNotToneMapped()
    {
        const auto px = toneMapPixel(0.0f, 0.0f, 0.0f, 0.5f);
        QCOMPARE(int(px.a), 128);
    }

    // shouldAttemptFp16Capture: the exact gate FramePumpService::
    // createSession uses. All three must be true; any one false must
    // refuse — this is what guarantees flag-off (or non-HDR, or
    // unsupported-GPU) never even considers the FP16 path.
    void fp16Gate_requiresAllThreeConditions_data()
    {
        QTest::addColumn<bool>("flagEnabled");
        QTest::addColumn<bool>("hdrActive");
        QTest::addColumn<bool>("fp16Supported");
        QTest::addColumn<bool>("expected");

        QTest::newRow("all true") << true << true << true << true;
        QTest::newRow("flag off") << false << true << true << false;
        QTest::newRow("display not HDR") << true << false << true << false;
        QTest::newRow("GPU unsupported") << true << true << false << false;
        QTest::newRow("all false") << false << false << false << false;
    }

    void fp16Gate_requiresAllThreeConditions()
    {
        QFETCH(bool, flagEnabled);
        QFETCH(bool, hdrActive);
        QFETCH(bool, fp16Supported);
        QFETCH(bool, expected);
        QCOMPARE(shouldAttemptFp16Capture(flagEnabled, hdrActive, fp16Supported), expected);
    }
};

QTEST_APPLESS_MAIN(TestHdrToneMap)
#include "tst_hdrtonemap.moc"
