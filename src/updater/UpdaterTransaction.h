#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace updater
{
struct Transaction
{
    int schemaVersion = 0;
    std::string productId;
    std::string expectedVersion;
    std::string expectedSha256;
    std::filesystem::path packageRoot;
    std::filesystem::path packagePath;
    std::filesystem::path stagingDir;
    std::filesystem::path backupDir;
    std::filesystem::path restartExecutable;
    std::filesystem::path healthTokenPath;
    std::filesystem::path dataDir;
    std::filesystem::path dataSnapshotDir;
    // Process id of the application that wrote this transaction. --apply waits
    // for it to exit before mutating any files.
    long long callerPid = 0;
    std::string phase;
};

bool loadAndValidateTransaction(const std::filesystem::path &transactionPath,
                                Transaction &transactionOut, std::string &errorOut);
std::vector<std::string> plannedOperations(const Transaction &transaction);
std::string pathToUtf8(const std::filesystem::path &path);
} // namespace updater
