#include "capture/hdr/HdrToneMapMath.h"

#include <algorithm>
#include <cmath>

namespace capture::hdr {

namespace {

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
    // Identity in [0,1] (SDR untouched), hard-clipped above — see the
    // DELIBERATE SIMPLIFICATION note in the header.
    r = std::clamp(r, 0.0f, 1.0f);
    g = std::clamp(g, 0.0f, 1.0f);
    b = std::clamp(b, 0.0f, 1.0f);

    SdrPixel out;
    out.r = quantize(srgbEncode(r));
    out.g = quantize(srgbEncode(g));
    out.b = quantize(srgbEncode(b));
    out.a = quantize(std::clamp(a, 0.0f, 1.0f));
    return out;
}

} // namespace capture::hdr
