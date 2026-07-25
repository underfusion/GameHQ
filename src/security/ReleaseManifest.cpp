#include "security/ReleaseManifest.h"

#include <algorithm>
#include <cctype>
#include <limits>

namespace
{
// A deliberately small strict JSON reader. The manifest schema is fixed and
// tiny, so a full parser would only add attack surface: this one refuses
// duplicate keys, trailing data, deep nesting and any number that is not a
// plain non-negative integer.
constexpr int kMaximumDepth = 8;

class Reader
{
public:
    Reader(const std::uint8_t *data, std::size_t size)
        : m_data(reinterpret_cast<const char *>(data)), m_size(size)
    {
    }

    bool atEnd() const { return m_pos >= m_size; }
    const std::string &error() const { return m_error; }

    bool fail(const char *reason)
    {
        if (m_error.empty())
            m_error = reason;
        return false;
    }

    void skipWhitespace()
    {
        while (m_pos < m_size) {
            const char ch = m_data[m_pos];
            if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r')
                ++m_pos;
            else
                break;
        }
    }

    bool expect(char ch)
    {
        skipWhitespace();
        if (m_pos >= m_size || m_data[m_pos] != ch)
            return fail("unexpected character in manifest JSON");
        ++m_pos;
        return true;
    }

    bool peek(char ch)
    {
        skipWhitespace();
        return m_pos < m_size && m_data[m_pos] == ch;
    }

    bool readString(std::string &out)
    {
        if (!expect('"'))
            return false;
        out.clear();
        while (true) {
            if (m_pos >= m_size)
                return fail("unterminated string in manifest JSON");
            const unsigned char ch = static_cast<unsigned char>(m_data[m_pos++]);
            if (ch == '"')
                return true;
            if (ch < 0x20)
                return fail("control character in manifest JSON string");
            if (ch != '\\') {
                out.push_back(static_cast<char>(ch));
                continue;
            }
            if (m_pos >= m_size)
                return fail("truncated escape in manifest JSON");
            const char escape = m_data[m_pos++];
            switch (escape) {
            case '"': out.push_back('"'); break;
            case '\\': out.push_back('\\'); break;
            case '/': out.push_back('/'); break;
            case 'b': out.push_back('\b'); break;
            case 'f': out.push_back('\f'); break;
            case 'n': out.push_back('\n'); break;
            case 'r': out.push_back('\r'); break;
            case 't': out.push_back('\t'); break;
            case 'u': {
                // The schema is ASCII-only; accept the escape form but refuse
                // anything that would decode outside plain ASCII rather than
                // growing a UTF-16 surrogate decoder here.
                if (m_pos + 4 > m_size)
                    return fail("truncated unicode escape in manifest JSON");
                unsigned value = 0;
                for (int i = 0; i < 4; ++i) {
                    const char digit = m_data[m_pos++];
                    const int nibble = digit >= '0' && digit <= '9' ? digit - '0'
                        : digit >= 'a' && digit <= 'f' ? digit - 'a' + 10
                        : digit >= 'A' && digit <= 'F' ? digit - 'A' + 10
                        : -1;
                    if (nibble < 0)
                        return fail("invalid unicode escape in manifest JSON");
                    value = (value << 4) | static_cast<unsigned>(nibble);
                }
                if (value == 0 || value > 0x7f)
                    return fail("non-ASCII escape in manifest JSON");
                out.push_back(static_cast<char>(value));
                break;
            }
            default:
                return fail("invalid escape in manifest JSON");
            }
        }
    }

    bool readUnsigned(std::uint64_t &out)
    {
        skipWhitespace();
        const std::size_t start = m_pos;
        while (m_pos < m_size && m_data[m_pos] >= '0' && m_data[m_pos] <= '9')
            ++m_pos;
        if (m_pos == start)
            return fail("expected a non-negative integer in manifest JSON");
        if (m_pos - start > 1 && m_data[start] == '0')
            return fail("leading zero in manifest JSON number");
        if (m_pos - start > 20)
            return fail("manifest JSON number is out of range");
        // Reject the fractional/exponent forms outright: every numeric field in
        // the schema is a count or a size.
        if (m_pos < m_size && (m_data[m_pos] == '.' || m_data[m_pos] == 'e' || m_data[m_pos] == 'E'))
            return fail("manifest JSON numbers must be integers");
        std::uint64_t value = 0;
        for (std::size_t i = start; i < m_pos; ++i) {
            const std::uint64_t digit = static_cast<std::uint64_t>(m_data[i] - '0');
            if (value > (std::numeric_limits<std::uint64_t>::max() - digit) / 10)
                return fail("manifest JSON number is out of range");
            value = value * 10 + digit;
        }
        out = value;
        return true;
    }

    bool finished()
    {
        skipWhitespace();
        return m_pos == m_size;
    }

private:
    const char *m_data;
    std::size_t m_size;
    std::size_t m_pos = 0;
    std::string m_error;
};

bool isLowercaseHex(const std::string &text, std::size_t length)
{
    return text.size() == length
        && std::all_of(text.begin(), text.end(), [](unsigned char ch) {
               return (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f');
           });
}

// File names are embedded in paths and compared against GitHub asset names, so
// keep them to the exact shape the generator produces.
bool isSafeFileName(const std::string &name)
{
    if (name.empty() || name.size() > 128)
        return false;
    if (name.front() == '.' || name.back() == '.')
        return false;
    return std::all_of(name.begin(), name.end(), [](unsigned char ch) {
        return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')
            || (ch >= '0' && ch <= '9') || ch == '.' || ch == '-' || ch == '_';
    });
}

bool isDottedVersion(const std::string &text)
{
    int digits = 0;
    int dots = 0;
    if (text.empty() || text.size() > 32)
        return false;
    for (char ch : text) {
        if (ch == '.') {
            if (digits == 0)
                return false;
            digits = 0;
            ++dots;
        } else if (ch >= '0' && ch <= '9') {
            if (++digits > 9)
                return false;
        } else {
            return false;
        }
    }
    return dots == 2 && digits > 0;
}

// "yyyy-MM-ddTHH:mm:ssZ" exactly, as emitted by the generator.
bool isUtcTimestamp(const std::string &text)
{
    static constexpr char kPattern[] = "####-##-##T##:##:##Z";
    if (text.size() != sizeof(kPattern) - 1)
        return false;
    for (std::size_t i = 0; i < text.size(); ++i) {
        const char expected = kPattern[i];
        if (expected == '#') {
            if (text[i] < '0' || text[i] > '9')
                return false;
        } else if (text[i] != expected) {
            return false;
        }
    }
    return true;
}
} // namespace

namespace release_manifest
{
const Artifact *Manifest::artifactOfKind(std::string_view kind) const
{
    const auto found = std::find_if(artifacts.begin(), artifacts.end(),
                                    [&](const Artifact &candidate) { return candidate.kind == kind; });
    return found == artifacts.end() ? nullptr : &*found;
}

bool normalizeSignatureText(std::string_view raw, std::string &signatureOut)
{
    if (raw.size() > kMaximumSignatureBytes)
        return false;
    std::size_t begin = 0;
    std::size_t end = raw.size();
    const auto isSpace = [](char ch) {
        return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
    };
    while (begin < end && isSpace(raw[begin]))
        ++begin;
    while (end > begin && isSpace(raw[end - 1]))
        --end;
    const std::string_view trimmed = raw.substr(begin, end - begin);
    // Strict form only: 86 Base64 characters plus "==" for a 64-byte signature.
    std::array<std::uint8_t, 64> decoded{};
    if (!release_trust::decodeStrictSignatureBase64(trimmed, decoded))
        return false;
    signatureOut.assign(trimmed);
    return true;
}

const std::vector<release_trust::TrustedKey> &trustedKeys()
{
    static const std::vector<release_trust::TrustedKey> keys = [] {
        std::vector<release_trust::TrustedKey> table;
#ifdef GAMEHQ_RELEASE_TRUST_TEST_KEYS
        // TEST ONLY. This is the RFC 8032 section 7.1 vector-1 public key, whose
        // private half is published in the RFC, so it authenticates nothing.
        // packaging/validate-release.ps1 refuses to publish a signed/Stable
        // build while this key is active; production activation is t48.
        release_trust::TrustedKey testKey;
        testKey.keyId = "gamehq-test-2026-01";
        if (release_trust::decodeStrictPublicKeyBase64(
                "11qYAYKxCrfVS/7TyWQHOg7hcvPapiMlrwIaaPcHURo=", testKey.publicKey)) {
            testKey.state = release_trust::KeyState::Current;
            testKey.minimumReleaseSequence = 1;
            table.push_back(testKey);
        }
#endif
        return table;
    }();
    return keys;
}

bool trustTableIsTestOnly()
{
    const auto &keys = trustedKeys();
    if (keys.empty())
        return true;
    return std::all_of(keys.begin(), keys.end(), [](const release_trust::TrustedKey &key) {
        return key.keyId.rfind("gamehq-test-", 0) == 0;
    });
}

bool parse(const std::vector<std::uint8_t> &manifestBytes, Manifest &manifestOut,
           std::string &errorOut)
{
    if (manifestBytes.empty() || manifestBytes.size() > kMaximumManifestBytes) {
        errorOut = "manifest size is out of range";
        return false;
    }

    Reader reader(manifestBytes.data(), manifestBytes.size());
    Manifest manifest;
    bool sawSchemaVersion = false;
    bool sawProductId = false;
    bool sawVersion = false;
    bool sawSequence = false;
    bool sawPublishedAt = false;
    bool sawMinimumUpdater = false;
    bool sawArtifacts = false;
    bool sawKeyId = false;

    if (!reader.expect('{')) {
        errorOut = reader.error();
        return false;
    }
    bool first = true;
    while (!reader.peek('}')) {
        if (!first && !reader.expect(',')) {
            errorOut = reader.error();
            return false;
        }
        first = false;
        std::string field;
        if (!reader.readString(field) || !reader.expect(':')) {
            errorOut = reader.error();
            return false;
        }

        const auto duplicate = [&](bool &flag) {
            if (flag) {
                reader.fail("duplicate field in manifest JSON");
                return true;
            }
            flag = true;
            return false;
        };

        if (field == "schemaVersion") {
            std::uint64_t value = 0;
            if (duplicate(sawSchemaVersion) || !reader.readUnsigned(value) || value > 1000) {
                errorOut = reader.error().empty() ? "invalid schemaVersion" : reader.error();
                return false;
            }
            manifest.schemaVersion = static_cast<int>(value);
        } else if (field == "productId") {
            if (duplicate(sawProductId) || !reader.readString(manifest.productId)) {
                errorOut = reader.error();
                return false;
            }
        } else if (field == "version") {
            if (duplicate(sawVersion) || !reader.readString(manifest.version)) {
                errorOut = reader.error();
                return false;
            }
        } else if (field == "releaseSequence") {
            if (duplicate(sawSequence) || !reader.readUnsigned(manifest.releaseSequence)) {
                errorOut = reader.error();
                return false;
            }
        } else if (field == "publishedAtUtc") {
            if (duplicate(sawPublishedAt) || !reader.readString(manifest.publishedAtUtc)) {
                errorOut = reader.error();
                return false;
            }
        } else if (field == "minimumUpdaterVersion") {
            if (duplicate(sawMinimumUpdater) || !reader.readString(manifest.minimumUpdaterVersion)) {
                errorOut = reader.error();
                return false;
            }
        } else if (field == "keyId") {
            if (duplicate(sawKeyId) || !reader.readString(manifest.keyId)) {
                errorOut = reader.error();
                return false;
            }
        } else if (field == "artifacts") {
            if (duplicate(sawArtifacts) || !reader.expect('[')) {
                errorOut = reader.error();
                return false;
            }
            bool firstArtifact = true;
            while (!reader.peek(']')) {
                if (!firstArtifact && !reader.expect(',')) {
                    errorOut = reader.error();
                    return false;
                }
                firstArtifact = false;
                if (manifest.artifacts.size() >= kMaximumDepth * 4) {
                    errorOut = "too many artifacts in manifest";
                    return false;
                }
                if (!reader.expect('{')) {
                    errorOut = reader.error();
                    return false;
                }
                Artifact artifact;
                bool sawKind = false;
                bool sawFileName = false;
                bool sawSize = false;
                bool sawSha = false;
                bool firstArtifactField = true;
                while (!reader.peek('}')) {
                    if (!firstArtifactField && !reader.expect(',')) {
                        errorOut = reader.error();
                        return false;
                    }
                    firstArtifactField = false;
                    std::string artifactField;
                    if (!reader.readString(artifactField) || !reader.expect(':')) {
                        errorOut = reader.error();
                        return false;
                    }
                    bool ok = false;
                    if (artifactField == "kind")
                        ok = !duplicate(sawKind) && reader.readString(artifact.kind);
                    else if (artifactField == "fileName")
                        ok = !duplicate(sawFileName) && reader.readString(artifact.fileName);
                    else if (artifactField == "size")
                        ok = !duplicate(sawSize) && reader.readUnsigned(artifact.size);
                    else if (artifactField == "sha256")
                        ok = !duplicate(sawSha) && reader.readString(artifact.sha256);
                    else
                        reader.fail("unknown field in manifest artifact");
                    if (!ok) {
                        errorOut = reader.error();
                        return false;
                    }
                }
                if (!reader.expect('}')) {
                    errorOut = reader.error();
                    return false;
                }
                if (!sawKind || !sawFileName || !sawSize || !sawSha) {
                    errorOut = "manifest artifact is missing required fields";
                    return false;
                }
                manifest.artifacts.push_back(std::move(artifact));
            }
            if (!reader.expect(']')) {
                errorOut = reader.error();
                return false;
            }
        } else {
            errorOut = "unknown field in manifest JSON";
            return false;
        }
    }
    if (!reader.expect('}')) {
        errorOut = reader.error();
        return false;
    }
    if (!reader.finished()) {
        errorOut = "trailing data after manifest JSON";
        return false;
    }

    if (!sawSchemaVersion || !sawProductId || !sawVersion || !sawSequence
        || !sawPublishedAt || !sawMinimumUpdater || !sawArtifacts || !sawKeyId) {
        errorOut = "manifest is missing required fields";
        return false;
    }
    if (manifest.schemaVersion != kSupportedSchemaVersion) {
        errorOut = "unsupported manifest schema version";
        return false;
    }
    if (manifest.productId != kProductId) {
        errorOut = "manifest is for a different product";
        return false;
    }
    if (!isDottedVersion(manifest.version)) {
        errorOut = "manifest version is not X.Y.Z";
        return false;
    }
    if (!isDottedVersion(manifest.minimumUpdaterVersion)) {
        errorOut = "manifest minimumUpdaterVersion is not X.Y.Z";
        return false;
    }
    if (manifest.releaseSequence == 0) {
        errorOut = "manifest release sequence must be positive";
        return false;
    }
    if (!isUtcTimestamp(manifest.publishedAtUtc)) {
        errorOut = "manifest publish timestamp is malformed";
        return false;
    }
    if (manifest.artifacts.empty()) {
        errorOut = "manifest lists no artifacts";
        return false;
    }
    for (std::size_t i = 0; i < manifest.artifacts.size(); ++i) {
        const Artifact &artifact = manifest.artifacts[i];
        if (artifact.kind != "setup" && artifact.kind != "portable" && artifact.kind != "update") {
            errorOut = "manifest artifact has an unknown kind";
            return false;
        }
        if (!isSafeFileName(artifact.fileName)) {
            errorOut = "manifest artifact file name is unsafe";
            return false;
        }
        if (artifact.size == 0 || artifact.size > (2ULL << 30)) {
            errorOut = "manifest artifact size is out of range";
            return false;
        }
        if (!isLowercaseHex(artifact.sha256, 64)) {
            errorOut = "manifest artifact hash is malformed";
            return false;
        }
        for (std::size_t j = 0; j < i; ++j) {
            if (manifest.artifacts[j].kind == artifact.kind
                || manifest.artifacts[j].fileName == artifact.fileName) {
                errorOut = "manifest lists the same artifact twice";
                return false;
            }
        }
        // The file name must carry the released version, so a manifest cannot
        // authorise an asset belonging to a different release.
        if (artifact.fileName.find(manifest.version) == std::string::npos) {
            errorOut = "manifest artifact does not belong to this version";
            return false;
        }
    }

    manifestOut = std::move(manifest);
    return true;
}

bool verifyAndParse(const std::vector<std::uint8_t> &manifestBytes,
                    std::string_view signatureText,
                    const release_trust::SequenceState *previousState,
                    AcceptedRelease &acceptedOut, std::string &errorOut)
{
    if (manifestBytes.empty() || manifestBytes.size() > kMaximumManifestBytes) {
        errorOut = "manifest size is out of range";
        return false;
    }
    std::string signature;
    if (!normalizeSignatureText(signatureText, signature)) {
        errorOut = "release signature is not canonical Base64 for 64 bytes";
        return false;
    }
    const auto &keys = trustedKeys();
    if (keys.empty()) {
        errorOut = "this build trusts no release signing key";
        return false;
    }

    // Step 1 - find which compiled key signs these exact bytes. The manifest's
    // own keyId is not consulted yet: it is unverified data at this point and
    // must never be able to steer key selection. Passing each key's own
    // minimum sequence here isolates this step to the cryptographic check;
    // activation and anti-rollback are enforced in step 3 with the real value.
    std::string signerKeyId;
    std::string manifestSha256;
    for (const release_trust::TrustedKey &key : keys) {
        if (key.state != release_trust::KeyState::Current)
            continue;
        const std::vector<release_trust::TrustedKey> single{key};
        const auto attempt = release_trust::verify(manifestBytes, signature, key.keyId,
                                                   key.minimumReleaseSequence, single, nullptr);
        if (attempt.accepted()) {
            signerKeyId = key.keyId;
            manifestSha256 = attempt.manifestSha256;
            break;
        }
    }
    if (signerKeyId.empty()) {
        errorOut = "no trusted key signs this release manifest";
        return false;
    }

    // Step 2 - only now is it safe to look at the contents.
    Manifest manifest;
    if (!parse(manifestBytes, manifest, errorOut))
        return false;
    if (manifest.keyId != signerKeyId) {
        errorOut = "manifest names a different signing key than the one that signed it";
        return false;
    }

    // Step 3 - re-run the reviewed decision with the manifest's real sequence so
    // key activation windows, rollback and equivocation all apply.
    const auto decision = release_trust::verify(manifestBytes, signature, manifest.keyId,
                                                manifest.releaseSequence, keys, previousState);
    if (!decision.accepted()) {
        errorOut = decision.error;
        return false;
    }
    if (decision.manifestSha256 != manifestSha256) {
        errorOut = "manifest hash changed between verification passes";
        return false;
    }

    acceptedOut.manifest = std::move(manifest);
    acceptedOut.manifestSha256 = decision.manifestSha256;
    acceptedOut.keyId = signerKeyId;
    return true;
}

bool artifactMatchesFile(const Artifact &artifact, const std::vector<std::uint8_t> &contents,
                         std::string &errorOut)
{
    if (contents.size() != artifact.size) {
        errorOut = "downloaded artifact size does not match the signed manifest";
        return false;
    }
    const std::string actual = release_trust::sha256Hex(
        contents.empty() ? nullptr : contents.data(), contents.size());
    if (actual.empty()) {
        errorOut = "could not hash the downloaded artifact";
        return false;
    }
    if (actual != artifact.sha256) {
        errorOut = "downloaded artifact hash does not match the signed manifest";
        return false;
    }
    return true;
}
} // namespace release_manifest
