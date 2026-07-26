#pragma once

#include <cstdint>

namespace release_trust_config
{
inline constexpr char kProductionKeyId[] = "gamehq-prod-2026-01";
inline constexpr char kProductionPublicKeyBase64[] = "c1HPtFeWwYv+Ey61AEhEL/guL8hsk8gHwqr/fsy0BOo=";
inline constexpr std::uint64_t kProductionMinimumReleaseSequence =
    25ULL;
} // namespace release_trust_config
