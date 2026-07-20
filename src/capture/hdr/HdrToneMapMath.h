#pragma once

namespace capture::hdr {

struct SdrPixel { unsigned char b, g, r, a; };

// Reference/paper white in scRGB terms — 1.0 is SDR white per the Windows
// FP16 desktop-capture convention. kneeStart is the fraction of reference
// white below which the curve is untouched identity (shadows/midtones
// preserved exactly); above it, highlights are compressed with a smooth,
// monotonic knee rather than clipped flat. See toneMapPixel() below.
inline constexpr float kReferenceWhite = 1.0f;
inline constexpr float kKneeStart = 0.9f * kReferenceWhite;

// Maps one scRGB/FP16 linear HDR pixel (r,g,b >= 0, SDR white = 1.0, HDR
// highlights may exceed 1.0) to an 8-bit sRGB BGRA pixel:
//   - identity for x <= kKneeStart (shadows/midtones untouched)
//   - f(x) = 1 - (1 - kKneeStart) * sqrt(kKneeStart / x)  for x > kKneeStart
// The knee is continuous in value (C0) at kKneeStart but not in slope (a
// deliberate, documented simplification — a fully filmic C1 curve needs
// real HDR content to tune, not available in this sandbox). It IS
// monotonically increasing for all x > 0 and asymptotes toward, but never
// reaches, full white, so highlights at 1.5x/2x/4x/8x/16x reference white
// all remain distinguishable instead of clipping to one flat value — see
// tst_hdrtonemap for the exact expected ordering.
//
// Negative input (wide-gamut BT.2020 colors outside sRGB primaries) is
// clipped to 0 — simple gamut clipping, not gamut mapping.
//
// This is the CPU oracle for GpuToneMapper's embedded HLSL (t24) — the two
// must stay numerically in lockstep; change both together.
SdrPixel toneMapPixel(float r, float g, float b, float a = 1.0f);

// Pure decision logic for whether FramePumpService::createSession should
// even attempt the FP16 experimental path, extracted so it is unit-testable
// without WGC/D3D — see tst_hdrtonemap. Mirrors the gate in createSession()
// exactly; keep both in lockstep.
bool shouldAttemptFp16Capture(bool experimentalFlagEnabled, bool displayHdrActive,
                              bool gpuFp16Supported);

} // namespace capture::hdr
