#include "updater/UpdaterRecovery.h"
#include "core/UpdateMaintenance.h"
#include "updater/UpdaterDataSnapshot.h"
#include "updater/UpdaterHealth.h"
#include "updater/UpdaterStaging.h"
#include "updater/UpdaterSwap.h"

#include <fstream>
#include <set>
#include <vector>
#include <windows.h>

namespace
{
std::filesystem::path phasePath(const updater::Transaction &tx)
{
    return tx.healthTokenPath.parent_path() / L"transaction.phase";
}

std::string readPhase(const updater::Transaction &tx)
{
    std::ifstream input(phasePath(tx), std::ios::binary);
    std::string phase;
    std::getline(input, phase);
    if (!phase.empty() && phase.back() == '\r') phase.pop_back();
    return phase;
}

std::wstring quoteArgument(const std::wstring &value)
{
    std::wstring quoted = L"\"";
    unsigned backslashes = 0;
    for (wchar_t ch : value) {
        if (ch == L'\\') ++backslashes;
        else if (ch == L'\"') {
            quoted.append(backslashes * 2 + 1, L'\\');
            quoted.push_back(ch);
            backslashes = 0;
        } else {
            quoted.append(backslashes, L'\\');
            backslashes = 0;
            quoted.push_back(ch);
        }
    }
    quoted.append(backslashes * 2, L'\\');
    quoted.push_back(L'\"');
    return quoted;
}

bool launchPrevious(const updater::Transaction &tx, std::string &error)
{
    std::wstring command = quoteArgument(tx.restartExecutable.wstring());
    std::vector<wchar_t> mutableCommand(command.begin(), command.end());
    mutableCommand.push_back(L'\0');
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(tx.restartExecutable.c_str(), mutableCommand.data(), nullptr, nullptr,
                        FALSE, 0, nullptr, tx.packageRoot.c_str(), &startup, &process)) {
        error = "rollback completed but the previous application could not be restarted (Windows error "
            + std::to_string(GetLastError()) + ")";
        return false;
    }
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return true;
}

bool rollbackAll(const updater::Transaction &tx, std::string &error)
{
    std::string dataError;
    std::string programError;
    bool dataOk = true;
    if (std::filesystem::is_directory(tx.dataSnapshotDir))
        dataOk = updater::restoreDataSnapshot(tx.dataDir, tx.dataSnapshotDir, dataError);
    bool programOk = true;
    if (updater::hasProgramSwapJournal(tx))
        programOk = updater::rollbackProgramFiles(tx, programError);
    if (!dataOk || !programOk) {
        error = (!dataOk ? dataError : std::string())
            + (!dataOk && !programOk ? "; " : std::string())
            + (!programOk ? programError : std::string());
        return false;
    }
    return true;
}
}

namespace updater
{
bool writeTransactionPhase(const Transaction &tx, const std::string &phase,
                           std::string &error)
{
    static const std::set<std::string> allowed = {
        "staged", "data_snapshotted", "swapped", "validating", "healthy", "rolled_back"
    };
    if (!allowed.contains(phase)) {
        error = "refusing to write an unknown update phase";
        return false;
    }
    const auto path = phasePath(tx);
    std::filesystem::path partial = path;
    partial += L".partial";
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    std::ofstream output(partial, std::ios::binary | std::ios::trunc);
    if (ec || !output) {
        error = "could not create the update phase marker";
        return false;
    }
    output << phase << '\n';
    output.flush();
    if (!output) {
        error = "could not finish the update phase marker";
        return false;
    }
    output.close();
    if (!MoveFileExW(partial.c_str(), path.c_str(),
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        std::filesystem::remove(partial, ec);
        error = "could not publish the update phase marker";
        return false;
    }
    return true;
}

bool applyUpdate(const Transaction &tx, int healthTimeoutMs, std::string &error)
{
    const std::string existing = readPhase(tx);
    if (hasProgramSwapJournal(tx) || std::filesystem::is_directory(tx.dataSnapshotDir)
        || (!existing.empty() && existing != "rolled_back")) {
        error = "an unfinished update requires recovery before another apply";
        return false;
    }
    std::error_code ec;
    std::filesystem::remove(phasePath(tx), ec);
    // With no journal/snapshot there is no live mutation to recover. A staging
    // directory is therefore only an abandoned extraction and is safe to purge.
    std::filesystem::remove_all(tx.stagingDir, ec);
    auto failAndRollback = [&](const std::string &cause) {
        std::string rollbackError;
        if (!rollbackAll(tx, rollbackError)) {
            error = cause + "; rollback incomplete: " + rollbackError;
            return false;
        }
        std::error_code cleanupError;
        std::filesystem::remove_all(tx.stagingDir, cleanupError);
        std::filesystem::remove_all(tx.dataSnapshotDir, cleanupError);
        if (!writeTransactionPhase(tx, "rolled_back", rollbackError)) {
            error = cause + "; rollback completed but its phase could not be recorded: " + rollbackError;
            return false;
        }
        maintenance::finish(tx.packageRoot);
        std::string launchError;
        if (!launchPrevious(tx, launchError)) {
            error = cause + "; " + launchError;
            return false;
        }
        error = cause + "; the previous version was restored and restarted";
        return false;
    };

    if (!extractAndValidatePackage(tx, error))
        return failAndRollback(error);
    if (!writeTransactionPhase(tx, "staged", error))
        return failAndRollback(error);
    if (!createDataSnapshot(tx.dataDir, tx.dataSnapshotDir, error))
        return failAndRollback(error);
    if (!writeTransactionPhase(tx, "data_snapshotted", error))
        return failAndRollback(error);
    if (!swapProgramFiles(tx, error))
        return failAndRollback(error);
    if (!writeTransactionPhase(tx, "swapped", error)
        || !writeTransactionPhase(tx, "validating", error))
        return failAndRollback(error);
    if (!launchAndWaitForHealth(tx, healthTimeoutMs, error))
        return failAndRollback(error);
    if (!writeTransactionPhase(tx, "healthy", error))
        return failAndRollback(error);
    maintenance::finish(tx.packageRoot);
    return true;
}

bool recoverInterruptedUpdate(const Transaction &tx, std::string &error)
{
    const std::string phase = readPhase(tx);
    if (phase == "healthy" || phase == "rolled_back")
        return true;
    if (phase.empty() && !hasProgramSwapJournal(tx)
        && !std::filesystem::is_directory(tx.dataSnapshotDir))
        return true;
    if (!rollbackAll(tx, error))
        return false;
    std::error_code ec;
    std::filesystem::remove_all(tx.stagingDir, ec);
    std::filesystem::remove_all(tx.dataSnapshotDir, ec);
    if (!writeTransactionPhase(tx, "rolled_back", error))
        return false;
    maintenance::finish(tx.packageRoot);
    return launchPrevious(tx, error);
}
}
