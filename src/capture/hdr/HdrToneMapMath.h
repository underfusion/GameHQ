#pragma once

namespace capture::hdr {

struct SdrPixel { unsigned char b, g, r, a; };

// Maps one scRGB/FP16 linear HDR pixel (r,g,b >= 0, SDR white = 1.0, HDR
// highlights may exceed 1.0) to an 8-bit sRGB BGRA pixel: identity/lossless
// for the SDR range [0,1], hard-clipped to white above it, then the sRGB
// OETF. Negative input (wide-gamut BT.2020 colors outside sRGB primaries) is
// clipped to 0 — simple gamut clipping, not gamut mapping.
//
// DELIBERATE SIMPLIFICATION: a perceptual curve (Reinhard/ACES/filmic) that
// also preserves SDR white exactly would need a "shoulder" starting below
// 1.0, which needs real HDR content to tune — not available in this
// sandbox. This clip-based curve is correctness-first (ordinary SDR content
// is untouched) at the cost of hard-clipped highlights; revisit once real
// hardware acceptance (t24/t22) is possible.
//
// This is the CPU oracle for GpuToneMapper's embedded HLSL (t24) — the two
// must stay numerically in lockstep; change both together.
//
// ASSUMPTION (unverified without real HDR hardware): the FP16 texture WGC
// hands back for an HDR-active desktop is scRGB linear light per the
// documented Windows convention. Flagged in t24/t22 as needing hardware
// acceptance before this is trusted end-to-end.
SdrPixel toneMapPixel(float r, float g, float b, float a = 1.0f);

} // namespace capture::hdr
