#include "updater/UpdaterDataSnapshot.h"

#include <array>
#include <fstream>
#include <set>
#include <system_error>
#include <vector>
#include <windows.h>

namespace
{
constexpr std::array<const wchar_t *, 4> kStateFiles = {
    L"config.json", L"gamehq.db", L"gamehq.db-wal", L"gamehq.db-shm"
};

bool safeRegularFile(const std::filesystem::path &path)
{
    const DWORD attributes = GetFileAttributesW(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES
        && !(attributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT));
}

bool readManifest(const std::filesystem::path &snapshotDir, std::set<std::wstring> &present,
                  std::string &error)
{
    std::ifstream manifest(snapshotDir / L"snapshot.manifest", std::ios::binary);
    std::string header;
    if (!std::getline(manifest, header) || header != "GAMEHQ_DATA_SNAPSHOT_V1") {
        error = "data snapshot manifest is missing or invalid";
        return false;
    }
    std::string line;
    while (std::getline(manifest, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        bool known = false;
        for (const wchar_t *name : kStateFiles) {
            std::filesystem::path candidate(line);
            if (candidate == std::filesystem::path(name)) {
                known = true;
                if (!present.insert(candidate.wstring()).second) {
                    error = "data snapshot manifest contains a duplicate file";
                    return false;
                }
                break;
            }
        }
        if (!known) {
            error = "data snapshot manifest contains an unexpected file";
            return false;
        }
    }
    return true;
}
} // namespace

namespace updater
{
bool createDataSnapshot(const std::filesystem::path &dataDir,
                        const std::filesystem::path &snapshotDir, std::string &error)
{
    std::error_code ec;
    if (!std::filesystem::is_directory(dataDir) || std::filesystem::exists(snapshotDir)) {
        error = "data directory is missing or snapshot already exists";
        return false;
    }
    if (!std::filesystem::create_directories(snapshotDir, ec) || ec) {
        error = "could not create data snapshot directory";
        return false;
    }
    const auto reject = [&](const std::string &reason) {
        std::error_code ignored;
        std::filesystem::remove_all(snapshotDir, ignored);
        error = reason;
        return false;
    };
    std::ofstream manifest(snapshotDir / L"snapshot.manifest", std::ios::binary | std::ios::trunc);
    if (!manifest)
        return reject("could not create data snapshot manifest");
    manifest << "GAMEHQ_DATA_SNAPSHOT_V1\n";
    for (const wchar_t *name : kStateFiles) {
        const std::filesystem::path source = dataDir / name;
        if (!std::filesystem::exists(source))
            continue;
        if (!safeRegularFile(source))
            return reject("data snapshot source is not a safe regular file");
        if (!std::filesystem::copy_file(source, snapshotDir / name,
                                        std::filesystem::copy_options::none, ec) || ec)
            return reject("could not copy a file into the data snapshot");
        manifest << std::filesystem::path(name).string() << '\n';
    }
    manifest.flush();
    if (!manifest)
        return reject("could not finish the data snapshot manifest");
    return true;
}

bool restoreDataSnapshot(const std::filesystem::path &dataDir,
                         const std::filesystem::path &snapshotDir, std::string &error)
{
    std::set<std::wstring> present;
    if (!readManifest(snapshotDir, present, error))
        return false;
    std::error_code ec;
    std::filesystem::create_directories(dataDir, ec);
    if (ec) {
        error = "could not open the data directory for restore";
        return false;
    }
    std::vector<std::filesystem::path> prepared;
    for (const wchar_t *name : kStateFiles) {
        if (!present.contains(name))
            continue;
        const std::filesystem::path source = snapshotDir / name;
        const std::filesystem::path partial = dataDir / (std::wstring(name) + L".restore.partial");
        if (!safeRegularFile(source)
            || !std::filesystem::copy_file(source, partial, std::filesystem::copy_options::overwrite_existing, ec)
            || ec) {
            for (const auto &path : prepared) std::filesystem::remove(path, ec);
            error = "could not prepare a data file for restore";
            return false;
        }
        prepared.push_back(partial);
    }

    // Rollback may only reverse operations this restore actually performed:
    // remove files it published, put back files it renamed away, drop its
    // partials. Untouched live files (e.g. a database whose backup step never
    // ran) must survive a failed restore.
    std::vector<std::pair<std::filesystem::path, std::filesystem::path>> backups;
    std::vector<std::filesystem::path> published;
    const auto rollback = [&] {
        std::error_code ignored;
        for (auto it = published.rbegin(); it != published.rend(); ++it)
            std::filesystem::remove(*it, ignored);
        for (auto it = backups.rbegin(); it != backups.rend(); ++it) {
            std::filesystem::remove(it->first, ignored);
            std::filesystem::rename(it->second, it->first, ignored);
        }
        for (const auto &path : prepared) std::filesystem::remove(path, ignored);
    };
    for (const wchar_t *name : kStateFiles) {
        const std::filesystem::path current = dataDir / name;
        if (!std::filesystem::exists(current))
            continue;
        if (!safeRegularFile(current)) {
            rollback();
            error = "current data state contains an unsafe file";
            return false;
        }
        const std::filesystem::path backup = dataDir / (std::wstring(name) + L".failed-update.backup");
        std::filesystem::remove(backup, ec);
        std::filesystem::rename(current, backup, ec);
        if (ec) {
            rollback();
            error = "could not preserve current data before restore";
            return false;
        }
        backups.emplace_back(current, backup);
    }
    for (const wchar_t *name : kStateFiles) {
        if (!present.contains(name))
            continue;
        const std::filesystem::path partial = dataDir / (std::wstring(name) + L".restore.partial");
        std::filesystem::rename(partial, dataDir / name, ec);
        if (ec) {
            rollback();
            error = "could not publish restored data state";
            return false;
        }
        published.push_back(dataDir / name);
    }
    // Also clears backups a crashed earlier restore may have left behind.
    for (const wchar_t *name : kStateFiles)
        std::filesystem::remove(dataDir / (std::wstring(name) + L".failed-update.backup"), ec);
    return true;
}
} // namespace updater
