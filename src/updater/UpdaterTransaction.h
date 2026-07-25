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

    // Signed-release evidence. The helper repeats the full Ed25519 check over
    // the manifest bytes on disk before extraction, so these fields are only a
    // binding: they never grant trust by themselves.
    std::filesystem::path manifestPath;
    std::filesystem::path signaturePath;
    std::string manifestSha256;
    std::string releaseSignature;
    std::string releaseKeyId;
    unsigned long long releaseSequence = 0;
    std::string artifactName;
    long long artifactSize = 0;
    std::string artifactSha256;

    std::string phase;
};

// Re-verifies the signed manifest referenced by the transaction and confirms it
// authorises exactly this package. Reads only; advances no trust state.
bool verifyReleaseAuthorisation(const Transaction &transaction, std::string &errorOut);

bool loadAndValidateTransaction(const std::filesystem::path &transactionPath,
                                Transaction &transactionOut, std::string &errorOut);
std::vector<std::string> plannedOperations(const Transaction &transaction);
std::string pathToUtf8(const std::filesystem::path &path);
} // namespace updater
