#include "capture/hdr/GpuToneMapper.h"
#include "capture/hdr/HdrToneMapMath.h"

#include <QTest>

#include <d3d11.h>

#include <cstring>

// Opt-in GPU smoke test for t24's tone-map stage (t22: "an opt-in GPU smoke
// test that skips cleanly when D3D11 support is unavailable"). This creates
// its own throwaway D3D11 device — separate from FramePumpService's — so it
// never touches the real capture path, and only needs a GPU capable of FP16
// textures, NOT a real HDR-active display. QSKIP is used, never QFAIL, when
// the environment can't run it (headless CI, ancient GPU, no D3D11 driver).
class TestHdrGpuToneMap : public QObject
{
    Q_OBJECT

private:
    ID3D11Device* m_device = nullptr;
    ID3D11DeviceContext* m_context = nullptr;

    static unsigned short floatToHalf(float value)
    {
        unsigned int f = 0;
        std::memcpy(&f, &value, sizeof(f));
        const unsigned int sign = (f >> 16) & 0x8000u;
        const int exp = int((f >> 23) & 0xFFu) - 127 + 15;
        const unsigned int mantissa = f & 0x7FFFFFu;

        if (exp <= 0)
            return static_cast<unsigned short>(sign); // flush tiny/zero to zero
        if (exp >= 0x1F)
            return static_cast<unsigned short>(sign | 0x7C00u); // overflow -> inf
        return static_cast<unsigned short>(sign | (unsigned(exp) << 10) | (mantissa >> 13));
    }

private slots:
    void init()
    {
        m_device = nullptr;
        m_context = nullptr;
        const D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1,
                                             D3D_FEATURE_LEVEL_10_0 };
        D3D_FEATURE_LEVEL got{};
        const HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
                                             levels, UINT(std::size(levels)), D3D11_SDK_VERSION,
                                             &m_device, &got, &m_context);
        if (FAILED(hr) || !m_device)
            QSKIP("No D3D11 hardware device available in this environment");
    }

    void cleanup()
    {
        if (m_context) { m_context->Release(); m_context = nullptr; }
        if (m_device)  { m_device->Release();  m_device = nullptr; }
    }

    void fp16_roundTripsThroughToneMapper_withinToleranceOfCpuOracle()
    {
        if (!capture::hdr::GpuToneMapper::checkFp16Support(m_device))
            QSKIP("GPU does not report FP16 texture/sample support");

        // 2x2 synthetic scRGB source: black, SDR white, 2x HDR highlight,
        // wide-gamut negative — matches the CPU oracle's own edge cases.
        struct Texel { float r, g, b, a; };
        const Texel src[4] = {
            { 0.0f, 0.0f, 0.0f, 1.0f },
            { 1.0f, 1.0f, 1.0f, 1.0f },
            { 2.0f, 2.0f, 2.0f, 1.0f },
            { -0.5f, 0.5f, 0.5f, 1.0f },
        };
        unsigned short halfData[4 * 4];
        for (int i = 0; i < 4; ++i) {
            halfData[i * 4 + 0] = floatToHalf(src[i].r);
            halfData[i * 4 + 1] = floatToHalf(src[i].g);
            halfData[i * 4 + 2] = floatToHalf(src[i].b);
            halfData[i * 4 + 3] = floatToHalf(src[i].a);
        }

        D3D11_TEXTURE2D_DESC srcDesc{};
        srcDesc.Width = 2;
        srcDesc.Height = 2;
        srcDesc.MipLevels = 1;
        srcDesc.ArraySize = 1;
        srcDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
        srcDesc.SampleDesc.Count = 1;
        srcDesc.Usage = D3D11_USAGE_DEFAULT;
        srcDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA initData{};
        initData.pSysMem = halfData;
        initData.SysMemPitch = 2 * 4 * sizeof(unsigned short);

        ID3D11Texture2D* srcTex = nullptr;
        HRESULT hr = m_device->CreateTexture2D(&srcDesc, &initData, &srcTex);
        QVERIFY2(SUCCEEDED(hr) && srcTex, "failed to create synthetic FP16 source texture");

        capture::hdr::GpuToneMapper mapper;
        QVERIFY2(mapper.init(m_device, m_context, 2, 2), "GpuToneMapper::init failed");

        ID3D11Texture2D* outTex = mapper.apply(srcTex);
        QVERIFY2(outTex, "GpuToneMapper::apply returned null");

        D3D11_TEXTURE2D_DESC stagingDesc{};
        stagingDesc.Width = 2;
        stagingDesc.Height = 2;
        stagingDesc.MipLevels = 1;
        stagingDesc.ArraySize = 1;
        stagingDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        stagingDesc.SampleDesc.Count = 1;
        stagingDesc.Usage = D3D11_USAGE_STAGING;
        stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

        ID3D11Texture2D* staging = nullptr;
        hr = m_device->CreateTexture2D(&stagingDesc, nullptr, &staging);
        QVERIFY2(SUCCEEDED(hr) && staging, "failed to create readback staging texture");
        m_context->CopyResource(staging, outTex);

        D3D11_MAPPED_SUBRESOURCE mapped{};
        hr = m_context->Map(staging, 0, D3D11_MAP_READ, 0, &mapped);
        QVERIFY2(SUCCEEDED(hr), "failed to map readback staging texture");

        const int tolerance = 3; // GPU pow()/sample vs. std::pow() CPU oracle
        for (int i = 0; i < 4; ++i) {
            const auto expected = capture::hdr::toneMapPixel(src[i].r, src[i].g, src[i].b, src[i].a);
            const auto* row = static_cast<const unsigned char*>(mapped.pData)
                             + (i / 2) * mapped.RowPitch + (i % 2) * 4;
            const int b = row[0], g = row[1], r = row[2], a = row[3];
            QVERIFY2(std::abs(r - int(expected.r)) <= tolerance, qPrintable(QStringLiteral(
                "texel %1 R mismatch: gpu=%2 cpu=%3").arg(i).arg(r).arg(expected.r)));
            QVERIFY2(std::abs(g - int(expected.g)) <= tolerance, qPrintable(QStringLiteral(
                "texel %1 G mismatch: gpu=%2 cpu=%3").arg(i).arg(g).arg(expected.g)));
            QVERIFY2(std::abs(b - int(expected.b)) <= tolerance, qPrintable(QStringLiteral(
                "texel %1 B mismatch: gpu=%2 cpu=%3").arg(i).arg(b).arg(expected.b)));
            QVERIFY2(std::abs(a - int(expected.a)) <= tolerance, qPrintable(QStringLiteral(
                "texel %1 A mismatch: gpu=%2 cpu=%3").arg(i).arg(a).arg(expected.a)));
        }

        m_context->Unmap(staging, 0);
        staging->Release();
        srcTex->Release();
    }
};

QTEST_APPLESS_MAIN(TestHdrGpuToneMap)
#include "tst_hdrgputonemap.moc"
