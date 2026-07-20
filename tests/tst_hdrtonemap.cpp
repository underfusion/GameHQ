#include "capture/hdr/HdrToneMapMath.h"

#include <QTest>

using capture::hdr::toneMapPixel;

// Pure-logic CPU oracle for the tone-map math shared with GpuToneMapper's
// embedded HLSL (t24). No D3D/GPU dependency, so this always runs in CI —
// the GPU-side smoke test lives separately and self-skips when unsupported.
class TestHdrToneMap : public QObject
{
    Q_OBJECT

private slots:
    // SDR white (1.0 scRGB) must map to exactly display white — this curve
    // is identity in the SDR range, so ordinary (non-HDR) content is
    // untouched, byte-for-byte, up to sRGB quantization.
    void sdrWhite_mapsToExactWhite()
    {
        const auto px = toneMapPixel(1.0f, 1.0f, 1.0f, 1.0f);
        QCOMPARE(int(px.r), 255);
        QCOMPARE(int(px.g), 255);
        QCOMPARE(int(px.b), 255);
        QCOMPARE(int(px.a), 255);
    }

    // Values in [0,1] must not brighten or darken relative to each other —
    // sRGB-encode alone is monotonic, so a mid-gray ramp must stay ordered.
    void ramp_isMonotonic()
    {
        int prev = -1;
        for (float v = 0.0f; v <= 1.0f; v += 0.1f) {
            const auto px = toneMapPixel(v, v, v);
            QVERIFY(int(px.r) >= prev);
            prev = int(px.r);
        }
    }

    // HDR highlights (values above 1.0, as scRGB represents brighter-than-
    // SDR-white content) hard-clip to white — see the DELIBERATE
    // SIMPLIFICATION note in HdrToneMapMath.h. Never wrap to black, never
    // exceed the 8-bit range.
    void hdrHighlight_clipsToWhite_data()
    {
        QTest::addColumn<float>("value");
        QTest::newRow("2x SDR white") << 2.0f;
        QTest::newRow("10x SDR white") << 10.0f;
        QTest::newRow("1000x SDR white") << 1000.0f;
    }

    void hdrHighlight_clipsToWhite()
    {
        QFETCH(float, value);
        const auto px = toneMapPixel(value, value, value);
        QCOMPARE(int(px.r), 255);
        QCOMPARE(int(px.g), 255);
        QCOMPARE(int(px.b), 255);
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

    void black_staysBlack()
    {
        const auto px = toneMapPixel(0.0f, 0.0f, 0.0f, 0.0f);
        QCOMPARE(int(px.r), 0);
        QCOMPARE(int(px.g), 0);
        QCOMPARE(int(px.b), 0);
        QCOMPARE(int(px.a), 0);
    }

    // Alpha is a plain linear-to-8-bit pass-through, not tone-mapped —
    // capture alpha is always opaque coverage, never HDR luminance.
    void alpha_isNotToneMapped()
    {
        const auto px = toneMapPixel(0.0f, 0.0f, 0.0f, 0.5f);
        QCOMPARE(int(px.a), 128);
    }
};

QTEST_APPLESS_MAIN(TestHdrToneMap)
#include "tst_hdrtonemap.moc"
