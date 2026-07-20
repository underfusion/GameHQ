#include "capture/hdr/GpuToneMapper.h"

#include <d3d11.h>
#include <d3dcompiler.h>

#include <QDebug>
#include <QString>

#include <cstring>

namespace capture::hdr {

namespace {

// SV_VertexID-only full-screen triangle — no vertex/index buffer needed.
const char* kVertexShaderSrc = R"(
struct VSOut { float4 pos : SV_POSITION; float2 uv : TEXCOORD0; };
VSOut VSMain(uint id : SV_VertexID)
{
    VSOut o;
    o.uv = float2((id << 1) & 2, id & 2);
    o.pos = float4(o.uv * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
    return o;
}
)";

// Identity in [0,1] (SDR untouched), hard-clipped above, then the sRGB
// OETF. Deliberate simplification, see HdrToneMapMath.h — must stay
// numerically in lockstep with the CPU oracle in HdrToneMapMath.cpp.
const char* kPixelShaderSrc = R"(
Texture2D<float4> HdrTex : register(t0);
SamplerState PointSampler : register(s0);

float SrgbEncode(float c)
{
    c = saturate(c);
    if (c <= 0.0031308) return c * 12.92;
    return 1.055 * pow(c, 1.0 / 2.4) - 0.055;
}

float4 PSMain(float4 pos : SV_POSITION, float2 uv : TEXCOORD0) : SV_TARGET
{
    float4 hdr = HdrTex.Sample(PointSampler, uv);
    float3 c = saturate(hdr.rgb);
    c = float3(SrgbEncode(c.r), SrgbEncode(c.g), SrgbEncode(c.b));
    return float4(c, saturate(hdr.a));
}
)";

ID3DBlob* compileShader(const char* src, const char* entry, const char* target)
{
    ID3DBlob* blob = nullptr;
    ID3DBlob* errors = nullptr;
    const HRESULT hr = D3DCompile(src, std::strlen(src), nullptr, nullptr, nullptr,
                                  entry, target, 0, 0, &blob, &errors);
    if (FAILED(hr)) {
        if (errors) {
            qWarning().noquote() << "GpuToneMapper: shader compile failed:"
                                 << QString::fromLatin1(
                                        static_cast<const char*>(errors->GetBufferPointer()),
                                        int(errors->GetBufferSize()));
            errors->Release();
        } else {
            qWarning().nospace() << "GpuToneMapper: shader compile failed hr=0x"
                                 << Qt::hex << quint32(hr);
        }
        return nullptr;
    }
    if (errors)
        errors->Release();
    return blob;
}

} // namespace

GpuToneMapper::~GpuToneMapper()
{
    releaseAll();
}

void GpuToneMapper::releaseAll()
{
    if (m_outputRtv)   { m_outputRtv->Release();   m_outputRtv = nullptr; }
    if (m_outputTex)   { m_outputTex->Release();   m_outputTex = nullptr; }
    if (m_rasterizer)  { m_rasterizer->Release();  m_rasterizer = nullptr; }
    if (m_sampler)     { m_sampler->Release();     m_sampler = nullptr; }
    if (m_ps)          { m_ps->Release();          m_ps = nullptr; }
    if (m_vs)          { m_vs->Release();          m_vs = nullptr; }
    m_device  = nullptr;
    m_context = nullptr;
    m_valid   = false;
}

bool GpuToneMapper::checkFp16Support(ID3D11Device* device)
{
    if (!device)
        return false;
    UINT support = 0;
    const HRESULT hr = device->CheckFormatSupport(DXGI_FORMAT_R16G16B16A16_FLOAT, &support);
    if (FAILED(hr))
        return false;
    const UINT needed = D3D11_FORMAT_SUPPORT_TEXTURE2D | D3D11_FORMAT_SUPPORT_SHADER_SAMPLE;
    return (support & needed) == needed;
}

bool GpuToneMapper::init(ID3D11Device* device, ID3D11DeviceContext* context,
                         unsigned width, unsigned height)
{
    releaseAll();
    if (!device || !context || width == 0 || height == 0)
        return false;

    ID3DBlob* vsBlob = compileShader(kVertexShaderSrc, "VSMain", "vs_4_0");
    if (!vsBlob)
        return false;
    ID3DBlob* psBlob = compileShader(kPixelShaderSrc, "PSMain", "ps_4_0");
    if (!psBlob) {
        vsBlob->Release();
        return false;
    }

    // No input layout: VSMain takes no per-vertex attributes (SV_VertexID
    // only, no bound vertex buffer), and CreateInputLayout(nullptr, 0, ...)
    // is rejected with E_INVALIDARG on this driver — IASetInputLayout(nullptr)
    // in apply() is the correct binding for this attributeless technique.
    HRESULT hr = device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
                                            nullptr, &m_vs);
    vsBlob->Release();
    if (FAILED(hr)) {
        qWarning().nospace() << "GpuToneMapper: CreateVertexShader failed hr=0x"
                             << Qt::hex << quint32(hr);
        psBlob->Release();
        releaseAll();
        return false;
    }

    hr = device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(),
                                   nullptr, &m_ps);
    psBlob->Release();
    if (FAILED(hr)) {
        qWarning().nospace() << "GpuToneMapper: CreatePixelShader failed hr=0x"
                             << Qt::hex << quint32(hr);
        releaseAll();
        return false;
    }

    D3D11_SAMPLER_DESC sampDesc{};
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    hr = device->CreateSamplerState(&sampDesc, &m_sampler);
    if (FAILED(hr)) {
        qWarning().nospace() << "GpuToneMapper: CreateSamplerState failed hr=0x"
                             << Qt::hex << quint32(hr);
        releaseAll();
        return false;
    }

    D3D11_RASTERIZER_DESC rastDesc{};
    rastDesc.FillMode = D3D11_FILL_SOLID;
    rastDesc.CullMode = D3D11_CULL_NONE;
    rastDesc.DepthClipEnable = TRUE;
    hr = device->CreateRasterizerState(&rastDesc, &m_rasterizer);
    if (FAILED(hr)) {
        qWarning().nospace() << "GpuToneMapper: CreateRasterizerState failed hr=0x"
                             << Qt::hex << quint32(hr);
        releaseAll();
        return false;
    }

    D3D11_TEXTURE2D_DESC texDesc{};
    texDesc.Width = width;
    texDesc.Height = height;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    hr = device->CreateTexture2D(&texDesc, nullptr, &m_outputTex);
    if (FAILED(hr)) {
        qWarning().nospace() << "GpuToneMapper: CreateTexture2D(output) failed hr=0x"
                             << Qt::hex << quint32(hr);
        releaseAll();
        return false;
    }

    hr = device->CreateRenderTargetView(m_outputTex, nullptr, &m_outputRtv);
    if (FAILED(hr)) {
        qWarning().nospace() << "GpuToneMapper: CreateRenderTargetView failed hr=0x"
                             << Qt::hex << quint32(hr);
        releaseAll();
        return false;
    }

    m_device  = device;
    m_context = context;
    m_width   = width;
    m_height  = height;
    m_valid   = true;
    return true;
}

ID3D11Texture2D* GpuToneMapper::apply(ID3D11Texture2D* fp16Src)
{
    if (!m_valid || !fp16Src)
        return nullptr;

    ID3D11ShaderResourceView* srv = nullptr;
    HRESULT hr = m_device->CreateShaderResourceView(fp16Src, nullptr, &srv);
    if (FAILED(hr) || !srv)
        return nullptr;

    ID3D11RenderTargetView* rtvs[] = { m_outputRtv };
    m_context->OMSetRenderTargets(1, rtvs, nullptr);

    D3D11_VIEWPORT vp{};
    vp.Width = float(m_width);
    vp.Height = float(m_height);
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    m_context->RSSetViewports(1, &vp);
    m_context->RSSetState(m_rasterizer);

    m_context->IASetInputLayout(nullptr);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_context->VSSetShader(m_vs, nullptr, 0);
    m_context->PSSetShader(m_ps, nullptr, 0);
    ID3D11ShaderResourceView* srvs[] = { srv };
    m_context->PSSetShaderResources(0, 1, srvs);
    ID3D11SamplerState* samplers[] = { m_sampler };
    m_context->PSSetSamplers(0, 1, samplers);

    m_context->Draw(3, 0);

    // Unbind so the next apply()'s SRV creation over a different pool buffer
    // never races a still-bound resource, and callers can freely read
    // m_outputTex afterwards (e.g. stage it into the recorder).
    ID3D11ShaderResourceView* nullSrv[] = { nullptr };
    m_context->PSSetShaderResources(0, 1, nullSrv);
    ID3D11RenderTargetView* nullRtv[] = { nullptr };
    m_context->OMSetRenderTargets(1, nullRtv, nullptr);

    srv->Release();
    return m_outputTex;
}

} // namespace capture::hdr
