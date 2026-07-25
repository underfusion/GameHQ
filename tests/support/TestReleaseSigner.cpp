#include "TestReleaseSigner.h"

#include <monocypher-ed25519.h>

#include <algorithm>
#include <array>

namespace
{
constexpr char kBase64Alphabet[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// RFC 8032 section 7.1 test 1 secret seed. Published in the RFC.
constexpr std::array<std::uint8_t, 32> kTestSeed = {
    0x9d, 0x61, 0xb1, 0x9d, 0xef, 0xfd, 0x5a, 0x60, 0xba, 0x84, 0x4a, 0xf4, 0x92, 0xec, 0x2c, 0xc4,
    0x44, 0x49, 0xc5, 0x69, 0x7b, 0x32, 0x69, 0x19, 0x70, 0x3b, 0xac, 0x03, 0x1c, 0xae, 0x7f, 0x60};

std::string toBase64(const std::uint8_t *bytes, std::size_t size)
{
    std::string out;
    out.reserve(((size + 2) / 3) * 4);
    for (std::size_t i = 0; i < size; i += 3) {
        const std::uint32_t word = static_cast<std::uint32_t>(bytes[i]) << 16
            | (i + 1 < size ? static_cast<std::uint32_t>(bytes[i + 1]) << 8 : 0)
            | (i + 2 < size ? bytes[i + 2] : 0);
        out.push_back(kBase64Alphabet[(word >> 18) & 63]);
        out.push_back(kBase64Alphabet[(word >> 12) & 63]);
        out.push_back(i + 1 < size ? kBase64Alphabet[(word >> 6) & 63] : '=');
        out.push_back(i + 2 < size ? kBase64Alphabet[word & 63] : '=');
    }
    return out;
}
} // namespace

namespace test_release_signer
{
std::string sign(const std::vector<std::uint8_t> &message)
{
    // crypto_ed25519_key_pair wipes the seed it is handed, so give it a copy.
    std::array<std::uint8_t, 32> seed = kTestSeed;
    std::array<std::uint8_t, 32> publicKey{};
    std::array<std::uint8_t, 64> secretKey{};
    crypto_ed25519_key_pair(secretKey.data(), publicKey.data(), seed.data());

    std::array<std::uint8_t, 64> signature{};
    crypto_ed25519_sign(signature.data(), secretKey.data(),
                        message.empty() ? nullptr : message.data(), message.size());
    return toBase64(signature.data(), signature.size());
}

std::vector<std::uint8_t> buildManifest(const std::string &version, std::uint64_t releaseSequence,
                                        const std::vector<ArtifactSpec> &artifacts,
                                        const std::string &keyId, const std::string &productId)
{
    std::string json = "{\"schemaVersion\":1,\"productId\":\"" + productId
        + "\",\"version\":\"" + version
        + "\",\"releaseSequence\":" + std::to_string(releaseSequence)
        + ",\"publishedAtUtc\":\"2026-07-25T00:00:00Z\",\"minimumUpdaterVersion\":\"" + version
        + "\",\"artifacts\":[";
    for (std::size_t i = 0; i < artifacts.size(); ++i) {
        if (i > 0)
            json += ',';
        json += "{\"kind\":\"" + artifacts[i].kind + "\",\"fileName\":\"" + artifacts[i].fileName
            + "\",\"size\":" + std::to_string(artifacts[i].size) + ",\"sha256\":\""
            + artifacts[i].sha256 + "\"}";
    }
    json += "],\"keyId\":\"" + keyId + "\"}\n";
    return {reinterpret_cast<const std::uint8_t *>(json.data()),
            reinterpret_cast<const std::uint8_t *>(json.data() + json.size())};
}
} // namespace test_release_signer
