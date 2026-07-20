#include "updater/UpdaterTransaction.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <map>
#include <regex>
#include <sstream>
#include <system_error>
#include <variant>
#include <windows.h>

namespace
{
using JsonValue = std::variant<std::string, long long>;

void appendUtf8(std::string &out, unsigned int codePoint)
{
    if (codePoint <= 0x7f) {
        out.push_back(static_cast<char>(codePoint));
    } else if (codePoint <= 0x7ff) {
        out.push_back(static_cast<char>(0xc0 | (codePoint >> 6)));
        out.push_back(static_cast<char>(0x80 | (codePoint & 0x3f)));
    } else if (codePoint <= 0xffff) {
        out.push_back(static_cast<char>(0xe0 | (codePoint >> 12)));
        out.push_back(static_cast<char>(0x80 | ((codePoint >> 6) & 0x3f)));
        out.push_back(static_cast<char>(0x80 | (codePoint & 0x3f)));
    } else {
        out.push_back(static_cast<char>(0xf0 | (codePoint >> 18)));
        out.push_back(static_cast<char>(0x80 | ((codePoint >> 12) & 0x3f)));
        out.push_back(static_cast<char>(0x80 | ((codePoint >> 6) & 0x3f)));
        out.push_back(static_cast<char>(0x80 | (codePoint & 0x3f)));
    }
}

class FlatJsonParser
{
public:
    explicit FlatJsonParser(const std::string &text) : m_text(text) {}

    bool parse(std::map<std::string, JsonValue> &values, std::string &error)
    {
        skipSpace();
        if (!take('{'))
            return fail(error, "transaction must be a JSON object");
        skipSpace();
        if (take('}'))
            return finish(error);
        for (;;) {
            std::string key;
            if (!parseString(key, error))
                return false;
            skipSpace();
            if (!take(':'))
                return fail(error, "expected ':' after transaction field");
            skipSpace();
            JsonValue value;
            if (peek() == '"') {
                std::string stringValue;
                if (!parseString(stringValue, error))
                    return false;
                value = std::move(stringValue);
            } else {
                long long number = 0;
                if (!parseInteger(number, error))
                    return false;
                value = number;
            }
            if (!values.emplace(std::move(key), std::move(value)).second)
                return fail(error, "transaction contains a duplicate field");
            skipSpace();
            if (take('}'))
                return finish(error);
            if (!take(','))
                return fail(error, "expected ',' between transaction fields");
            skipSpace();
        }
    }

private:
    char peek() const { return m_pos < m_text.size() ? m_text[m_pos] : '\0'; }
    bool take(char expected)
    {
        if (peek() != expected)
            return false;
        ++m_pos;
        return true;
    }
    void skipSpace()
    {
        while (m_pos < m_text.size() && std::isspace(static_cast<unsigned char>(m_text[m_pos])))
            ++m_pos;
    }
    bool finish(std::string &error)
    {
        skipSpace();
        return m_pos == m_text.size() || fail(error, "unexpected data after transaction object");
    }
    bool fail(std::string &error, const char *message)
    {
        error = message;
        return false;
    }
    bool parseHex4(unsigned int &value, std::string &error)
    {
        if (m_text.size() - m_pos < 4)
            return fail(error, "incomplete Unicode escape in transaction");
        value = 0;
        for (int i = 0; i < 4; ++i) {
            const char ch = m_text[m_pos++];
            value <<= 4;
            if (ch >= '0' && ch <= '9') value |= static_cast<unsigned int>(ch - '0');
            else if (ch >= 'a' && ch <= 'f') value |= static_cast<unsigned int>(ch - 'a' + 10);
            else if (ch >= 'A' && ch <= 'F') value |= static_cast<unsigned int>(ch - 'A' + 10);
            else return fail(error, "invalid Unicode escape in transaction");
        }
        return true;
    }
    bool parseString(std::string &out, std::string &error)
    {
        if (!take('"'))
            return fail(error, "expected a quoted transaction field");
        while (m_pos < m_text.size()) {
            const unsigned char ch = static_cast<unsigned char>(m_text[m_pos++]);
            if (ch == '"')
                return true;
            if (ch < 0x20)
                return fail(error, "control character in transaction string");
            if (ch != '\\') {
                out.push_back(static_cast<char>(ch));
                continue;
            }
            if (m_pos >= m_text.size())
                return fail(error, "incomplete escape in transaction string");
            const char escaped = m_text[m_pos++];
            switch (escaped) {
            case '"': out.push_back('"'); break;
            case '\\': out.push_back('\\'); break;
            case '/': out.push_back('/'); break;
            case 'b': out.push_back('\b'); break;
            case 'f': out.push_back('\f'); break;
            case 'n': out.push_back('\n'); break;
            case 'r': out.push_back('\r'); break;
            case 't': out.push_back('\t'); break;
            case 'u': {
                unsigned int codePoint = 0;
                if (!parseHex4(codePoint, error))
                    return false;
                if (codePoint >= 0xd800 && codePoint <= 0xdbff) {
                    if (m_text.size() - m_pos < 6 || m_text[m_pos] != '\\' || m_text[m_pos + 1] != 'u')
                        return fail(error, "unpaired Unicode surrogate in transaction");
                    m_pos += 2;
                    unsigned int low = 0;
                    if (!parseHex4(low, error) || low < 0xdc00 || low > 0xdfff)
                        return fail(error, "invalid Unicode surrogate pair in transaction");
                    codePoint = 0x10000 + ((codePoint - 0xd800) << 10) + (low - 0xdc00);
                } else if (codePoint >= 0xdc00 && codePoint <= 0xdfff) {
                    return fail(error, "unpaired Unicode surrogate in transaction");
                }
                appendUtf8(out, codePoint);
                break;
            }
            default: return fail(error, "invalid escape in transaction string");
            }
        }
        return fail(error, "unterminated transaction string");
    }
    bool parseInteger(long long &out, std::string &error)
    {
        const std::size_t begin = m_pos;
        if (peek() == '-')
            ++m_pos;
        while (std::isdigit(static_cast<unsigned char>(peek())))
            ++m_pos;
        if (m_pos == begin || (m_pos == begin + 1 && m_text[begin] == '-'))
            return fail(error, "transaction values must be strings or integers");
        try {
            out = std::stoll(m_text.substr(begin, m_pos - begin));
        } catch (...) {
            return fail(error, "transaction integer is out of range");
        }
        return true;
    }

    const std::string &m_text;
    std::size_t m_pos = 0;
};

bool utf8ToPath(const std::string &value, std::filesystem::path &pathOut)
{
    if (value.empty())
        return false;
    const int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
                                           static_cast<int>(value.size()), nullptr, 0);
    if (length <= 0)
        return false;
    std::wstring wide(static_cast<std::size_t>(length), L'\0');
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
                            static_cast<int>(value.size()), wide.data(), length) != length)
        return false;
    pathOut = std::filesystem::path(wide);
    return true;
}

bool getString(const std::map<std::string, JsonValue> &values, const char *key,
               std::string &out, std::string &error)
{
    const auto it = values.find(key);
    if (it == values.end() || !std::holds_alternative<std::string>(it->second)) {
        error = std::string("missing or invalid transaction field: ") + key;
        return false;
    }
    out = std::get<std::string>(it->second);
    return !out.empty() || (error = std::string("empty transaction field: ") + key, false);
}

bool getPath(const std::map<std::string, JsonValue> &values, const char *key,
             std::filesystem::path &out, std::string &error)
{
    std::string utf8;
    if (!getString(values, key, utf8, error))
        return false;
    if (!utf8ToPath(utf8, out)) {
        error = std::string("invalid UTF-8 path in transaction field: ") + key;
        return false;
    }
    return true;
}

std::filesystem::path normalizedPath(const std::filesystem::path &path, std::string &error)
{
    if (!path.is_absolute()) {
        error = "transaction paths must be absolute";
        return {};
    }
    std::error_code ec;
    const std::filesystem::path normalized = std::filesystem::weakly_canonical(path, ec);
    if (ec) {
        error = "could not canonicalize transaction path";
        return {};
    }
    return normalized;
}

bool pathWithin(const std::filesystem::path &root, const std::filesystem::path &candidate,
                bool allowRoot)
{
    auto rootIt = root.begin();
    auto candidateIt = candidate.begin();
    for (; rootIt != root.end(); ++rootIt, ++candidateIt) {
        if (candidateIt == candidate.end() || _wcsicmp(rootIt->c_str(), candidateIt->c_str()) != 0)
            return false;
    }
    return allowRoot || candidateIt != candidate.end();
}
} // namespace

namespace updater
{
std::string pathToUtf8(const std::filesystem::path &path)
{
    const std::wstring wide = path.wstring();
    if (wide.empty())
        return {};
    const int length = WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()),
                                           nullptr, 0, nullptr, nullptr);
    std::string result(static_cast<std::size_t>(length), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()),
                        result.data(), length, nullptr, nullptr);
    return result;
}

bool loadAndValidateTransaction(const std::filesystem::path &transactionPath,
                                Transaction &out, std::string &error)
{
    std::ifstream input(transactionPath, std::ios::binary);
    if (!input) {
        error = "could not open update transaction";
        return false;
    }
    std::ostringstream buffer;
    buffer << input.rdbuf();
    if (!input.good() && !input.eof()) {
        error = "could not read update transaction";
        return false;
    }
    const std::string json = buffer.str();
    if (json.size() > 64 * 1024) {
        error = "update transaction exceeds 64 KiB";
        return false;
    }

    std::map<std::string, JsonValue> values;
    if (!FlatJsonParser(json).parse(values, error))
        return false;
    const auto schema = values.find("schemaVersion");
    if (schema == values.end() || !std::holds_alternative<long long>(schema->second)
        || std::get<long long>(schema->second) != 1) {
        error = "unsupported update transaction schema";
        return false;
    }
    out.schemaVersion = 1;
    if (!getString(values, "productId", out.productId, error)
        || !getString(values, "expectedVersion", out.expectedVersion, error)
        || !getString(values, "expectedSha256", out.expectedSha256, error)
        || !getPath(values, "packageRoot", out.packageRoot, error)
        || !getPath(values, "packagePath", out.packagePath, error)
        || !getPath(values, "stagingDir", out.stagingDir, error)
        || !getPath(values, "backupDir", out.backupDir, error)
        || !getPath(values, "restartExecutable", out.restartExecutable, error)
        || !getPath(values, "healthTokenPath", out.healthTokenPath, error)
        || !getPath(values, "dataDir", out.dataDir, error)
        || !getPath(values, "dataSnapshotDir", out.dataSnapshotDir, error)
        || !getString(values, "phase", out.phase, error))
        return false;

    if (out.productId != "underfusion.gamehq") {
        error = "transaction product does not match GameHQ";
        return false;
    }
    if (!std::regex_match(out.expectedVersion, std::regex(R"([0-9]+\.[0-9]+\.[0-9]+)"))) {
        error = "transaction contains an invalid target version";
        return false;
    }
    if (!std::regex_match(out.expectedSha256, std::regex(R"([0-9A-Fa-f]{64})"))) {
        error = "transaction contains an invalid SHA-256 digest";
        return false;
    }
    std::transform(out.expectedSha256.begin(), out.expectedSha256.end(),
                   out.expectedSha256.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    if (out.phase != "download_verified") {
        error = "transaction is not ready for updater validation";
        return false;
    }

    out.packageRoot = normalizedPath(out.packageRoot, error);
    if (out.packageRoot.empty() || !std::filesystem::is_directory(out.packageRoot)) {
        if (error.empty()) error = "package root does not exist";
        return false;
    }
    std::filesystem::path transaction = normalizedPath(transactionPath, error);
    out.packagePath = normalizedPath(out.packagePath, error);
    out.stagingDir = normalizedPath(out.stagingDir, error);
    out.backupDir = normalizedPath(out.backupDir, error);
    out.restartExecutable = normalizedPath(out.restartExecutable, error);
    out.healthTokenPath = normalizedPath(out.healthTokenPath, error);
    out.dataDir = normalizedPath(out.dataDir, error);
    out.dataSnapshotDir = normalizedPath(out.dataSnapshotDir, error);
    if (!error.empty())
        return false;
    for (const auto &candidate : { transaction, out.packagePath, out.stagingDir, out.backupDir,
                                   out.restartExecutable, out.healthTokenPath, out.dataSnapshotDir }) {
        if (!pathWithin(out.packageRoot, candidate, false)) {
            error = "transaction path escapes the package root: " + pathToUtf8(candidate);
            return false;
        }
    }
    if (out.stagingDir == out.backupDir) {
        error = "staging and backup directories must be different";
        return false;
    }
    const std::string expectedName = "GameHQ-" + out.expectedVersion + "-win64-update.zip";
    if (pathToUtf8(out.packagePath.filename()) != expectedName) {
        error = "transaction package name does not match the target version";
        return false;
    }
    if (!std::filesystem::is_regular_file(out.packagePath)) {
        error = "verified update package is missing";
        return false;
    }
    return true;
}

std::vector<std::string> plannedOperations(const Transaction &tx)
{
    const std::vector<std::string> ownedPaths = {
        "GameHQ.exe", "app/", "README.txt", "LICENSE.txt",
        "THIRD_PARTY_NOTICES.md", "licenses/", "GameHQUpdater.pending.exe"
    };
    std::vector<std::string> operations;
    operations.push_back("VERIFY SHA-256 " + pathToUtf8(tx.packagePath));
    operations.push_back("CREATE STAGING " + pathToUtf8(tx.stagingDir));
    operations.push_back("EXTRACT PACKAGE " + pathToUtf8(tx.packagePath) + " -> " + pathToUtf8(tx.stagingDir));
    operations.push_back("CREATE BACKUP " + pathToUtf8(tx.backupDir));
    operations.push_back("SNAPSHOT DATA " + pathToUtf8(tx.dataDir) + " -> " + pathToUtf8(tx.dataSnapshotDir));
    for (const std::string &path : ownedPaths) {
        operations.push_back("BACKUP IF PRESENT " + path);
        operations.push_back("INSTALL IF PRESENT " + path);
    }
    operations.push_back("LAUNCH " + pathToUtf8(tx.restartExecutable)
                         + " --post-update " + tx.expectedVersion + " " + pathToUtf8(tx.healthTokenPath));
    return operations;
}
} // namespace updater
