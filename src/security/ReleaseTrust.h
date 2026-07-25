#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace release_trust
{
enum class KeyState { Current, Next, Revoked };

struct TrustedKey
{
    std::string keyId;
    std::array<std::uint8_t, 32> publicKey{};
    KeyState state = KeyState::Revoked;
    std::uint64_t minimumReleaseSequence = 0;
};

struct SequenceState
{
    std::uint64_t highestReleaseSequence = 0;
    std::string manifestSha256;
};

enum class VerifyCode
{
    Accepted,
    InvalidKeyId,
    UnknownKey,
    InactiveKey,
    RevokedKey,
    InvalidSignatureEncoding,
    InvalidSignature,
    Rollback,
    Equivocation,
    StateError
};

struct VerifyResult
{
    VerifyCode code = VerifyCode::InvalidSignature;
    std::string error;
    std::string manifestSha256;
    bool accepted() const { return code == VerifyCode::Accepted; }
};

bool decodeStrictSignatureBase64(std::string_view encoded,
                                 std::array<std::uint8_t, 64> &signatureOut);
bool decodeStrictPublicKeyBase64(std::string_view encoded,
                                 std::array<std::uint8_t, 32> &keyOut);
std::string sha256Hex(const std::uint8_t *data, std::size_t size);

VerifyResult verify(const std::vector<std::uint8_t> &manifestBytes,
                    std::string_view signatureBase64,
                    std::string_view keyId,
                    std::uint64_t releaseSequence,
                    const std::vector<TrustedKey> &trustedKeys,
                    const SequenceState *previousState = nullptr);

bool loadSequenceState(const std::filesystem::path &path, SequenceState &stateOut,
                       std::string &errorOut);
bool storeSequenceStateAtomically(const std::filesystem::path &path,
                                  const SequenceState &state, std::string &errorOut);

// Executes immutable RFC 8032 vectors inside any binary that links this
// library. Used by both the Qt test target and GameHQUpdater --release-trust-self-test.
bool runBuiltInSelfTest(std::string &errorOut);
} // namespace release_trust
