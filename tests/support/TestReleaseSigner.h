#pragma once

#include <cstdint>
#include <string>
#include <vector>

// TEST ONLY. Signs release manifests with the RFC 8032 section 7.1 vector-1
// private key, whose seed is published in the RFC and therefore authenticates
// nothing. This lives under tests/ so no shipped binary ever links signing
// code (docs/release-manifest-security-review.md).
namespace test_release_signer
{
inline constexpr char kTestKeyId[] = "gamehq-test-2026-01";

// Canonical padded Base64 of the Ed25519 signature over the exact bytes.
std::string sign(const std::vector<std::uint8_t> &message);

// Builds manifest bytes in the same field order the production generator uses.
struct ArtifactSpec
{
    std::string kind;
    std::string fileName;
    std::uint64_t size = 0;
    std::string sha256;
};

std::vector<std::uint8_t> buildManifest(const std::string &version, std::uint64_t releaseSequence,
                                        const std::vector<ArtifactSpec> &artifacts,
                                        const std::string &keyId = kTestKeyId,
                                        const std::string &productId = "underfusion.gamehq");
} // namespace test_release_signer
