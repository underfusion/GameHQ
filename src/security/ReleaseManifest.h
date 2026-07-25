#pragma once

#include "security/ReleaseTrust.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

// Strict reader for the signed release manifest produced by
// tools/release-manifest/GameHQ.ReleaseManifest.Tool. Deliberately free of Qt
// so the Qt application and the static GameHQUpdater helper share one
// implementation (docs/release-manifest-security-review.md).
//
// The contract is fail-closed: nothing here parses JSON until an Ed25519
// signature over the untouched download bytes has been accepted by a key that
// was compiled into the binary. A manifest can never introduce, promote or
// restore trust in a key.
namespace release_manifest
{
inline constexpr char kProductId[] = "underfusion.gamehq";
inline constexpr int kSupportedSchemaVersion = 1;

// Download caps. The generator emits well under a kilobyte; these bounds only
// exist so a hostile endpoint cannot stream an unbounded body at us.
inline constexpr std::size_t kMaximumManifestBytes = 64 * 1024;
inline constexpr std::size_t kMaximumSignatureBytes = 256;

// Artifact kinds the manifest may describe. The updater only ever installs
// "update"; the others are listed so the release page can be validated.
struct Artifact
{
    std::string kind;
    std::string fileName;
    std::uint64_t size = 0;
    std::string sha256; // lowercase hex, 64 characters
};

struct Manifest
{
    int schemaVersion = 0;
    std::string productId;
    std::string version;
    std::uint64_t releaseSequence = 0;
    std::string publishedAtUtc;
    std::string minimumUpdaterVersion;
    std::vector<Artifact> artifacts;
    std::string keyId;

    const Artifact *artifactOfKind(std::string_view kind) const;
};

struct AcceptedRelease
{
    Manifest manifest;
    std::string manifestSha256; // hash of the exact signed bytes
    std::string keyId;          // key that actually verified the signature
};

// Trims the surrounding whitespace a .sig file may carry and rejects anything
// that is not exactly one padded Base64 Ed25519 signature.
bool normalizeSignatureText(std::string_view raw, std::string &signatureOut);

// Immutable trust table shared by every C++ consumer. Built from the same
// public-key input as the C# table; a manifest never adds to it.
const std::vector<release_trust::TrustedKey> &trustedKeys();

// True when the active trust table only contains test keys. Release gates use
// this to refuse to publish a Stable build (see t48).
bool trustTableIsTestOnly();

// Parses the strict generator output. Only call this on bytes whose signature
// already verified.
bool parse(const std::vector<std::uint8_t> &manifestBytes, Manifest &manifestOut,
           std::string &errorOut);

// The whole fail-closed pipeline: locate the compiled key that signs these
// exact bytes, verify, parse, then re-check key activation, anti-rollback and
// equivocation with the manifest's own release sequence.
//
// previousState may be null for a stateless check (for example a helper
// re-verification that must not advance any counter).
bool verifyAndParse(const std::vector<std::uint8_t> &manifestBytes,
                    std::string_view signatureText,
                    const release_trust::SequenceState *previousState,
                    AcceptedRelease &acceptedOut, std::string &errorOut);

// Confirms a downloaded file is exactly the artifact the manifest authorises.
bool artifactMatchesFile(const Artifact &artifact, const std::vector<std::uint8_t> &contents,
                         std::string &errorOut);
} // namespace release_manifest
