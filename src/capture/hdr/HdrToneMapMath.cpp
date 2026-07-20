#include "capture/hdr/HdrToneMapMath.h"

#include <algorithm>
#include <cmath>

namespace capture::hdr {

namespace {

// Identity below the knee, then a monotonically increasing, asymptotic
// (never-quite-reaches-1.0) shoulder above it. Must stay numerically in
// lockstep with GpuToneMapper.cpp's embedded HLSL.
float shoulderCurve(float x)
{
    if (x <= kKneeStart)
        return x;
    return 1.0f - (1.0f - kKneeStart) * std::sqrt(kKneeStart / x);
}

float srgbEncode(float linear)
{
    linear = std::clamp(linear, 0.0f, 1.0f);
    if (linear <= 0.0031308f)
        return linear * 12.92f;
    return 1.055f * std::pow(linear, 1.0f / 2.4f) - 0.055f;
}

unsigned char quantize(float srgb)
{
    return static_cast<unsigned char>(std::lround(std::clamp(srgb, 0.0f, 1.0f) * 255.0f));
}

} // namespace

SdrPixel toneMapPixel(float r, float g, float b, float a)
{
    r = std::max(r, 0.0f);
    g = std::max(g, 0.0f);
    b = std::max(b, 0.0f);

    SdrPixel out;
    out.r = quantize(srgbEncode(shoulderCurve(r)));
    out.g = quantize(srgbEncode(shoulderCurve(g)));
    out.b = quantize(srgbEncode(shoulderCurve(b)));
    out.a = quantize(std::clamp(a, 0.0f, 1.0f));
    return out;
}

bool shouldAttemptFp16Capture(bool experimentalFlagEnabled, bool displayHdrActive,
                              bool gpuFp16Supported)
{
    return experimentalFlagEnabled && displayHdrActive && gpuFp16Supported;
}

} // namespace capture::hdr
