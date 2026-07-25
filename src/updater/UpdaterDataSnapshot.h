#pragma once

#include <filesystem>
#include <string>

namespace updater
{
bool createDataSnapshot(const std::filesystem::path &dataDir,
                        const std::filesystem::path &snapshotDir, std::string &errorOut);
bool restoreDataSnapshot(const std::filesystem::path &dataDir,
                         const std::filesystem::path &snapshotDir, std::string &errorOut);
} // namespace updater
