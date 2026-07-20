#include "updater/UpdaterTransaction.h"
#include "updater/UpdaterStaging.h"
#include "updater/UpdaterDataSnapshot.h"
#include "updater/UpdaterSwap.h"
#include "updater/UpdaterHealth.h"
#include "updater/UpdaterRecovery.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <windows.h>

namespace
{
struct HandleCloser { void operator()(void *handle) const { if (handle) CloseHandle(handle); } };
using UniqueHandle = std::unique_ptr<void, HandleCloser>;

std::wstring mutexNameFor(const std::filesystem::path &transactionPath)
{
    std::wstring normalized = std::filesystem::absolute(transactionPath).lexically_normal().wstring();
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), towlower);
    std::uint64_t hash = 1469598103934665603ULL;
    for (wchar_t ch : normalized) {
        hash ^= static_cast<std::uint64_t>(ch);
        hash *= 1099511628211ULL;
    }
    return L"Local\\GameHQUpdater-" + std::to_wstring(hash);
}

int runTransaction(const std::filesystem::path &transactionPath, const std::wstring &mode)
{
    UniqueHandle activeMutex(CreateMutexW(nullptr, TRUE, L"Local\\GameHQUpdaterActive"));
    if (!activeMutex || GetLastError() == ERROR_ALREADY_EXISTS) {
        std::cerr << "ERROR: another updater helper is already active\n";
        return 4;
    }
    const std::wstring mutexName = mutexNameFor(transactionPath);
    UniqueHandle mutex(CreateMutexW(nullptr, TRUE, mutexName.c_str()));
    if (!mutex) {
        std::cerr << "ERROR: could not create updater transaction mutex\n";
        return 3;
    }
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        std::cerr << "ERROR: another updater is already processing this transaction\n";
        return 4;
    }

    updater::Transaction transaction;
    std::string error;
    if (!updater::loadAndValidateTransaction(transactionPath, transaction, error)) {
        std::cerr << "ERROR: " << error << '\n';
        return 2;
    }
    if (mode == L"--dry-run") {
        std::cout << "DRY RUN - no files will be changed\n";
        std::cout << "TRANSACTION version=" << transaction.expectedVersion
                  << " phase=" << transaction.phase << '\n';
        int index = 1;
        for (const std::string &operation : updater::plannedOperations(transaction))
            std::cout << "PLAN " << index++ << ": " << operation << '\n';
        return 0;
    }
    if (mode == L"--snapshot-data") {
        if (!updater::createDataSnapshot(transaction.dataDir, transaction.dataSnapshotDir, error)) {
            std::cerr << "ERROR: " << error << '\n';
            return 6;
        }
        std::cout << "DATA SNAPSHOT READY\n";
        return 0;
    }
    if (mode == L"--restore-data") {
        if (!updater::restoreDataSnapshot(transaction.dataDir, transaction.dataSnapshotDir, error)) {
            std::cerr << "ERROR: " << error << '\n';
            return 7;
        }
        std::cout << "DATA SNAPSHOT RESTORED\n";
        return 0;
    }
    if (mode == L"--swap") {
        if (!updater::swapProgramFiles(transaction, error)) {
            std::cerr << "ERROR: " << error << '\n';
            return 8;
        }
        std::cout << "PROGRAM FILES SWAPPED\n";
        return 0;
    }
    if (mode == L"--wait-health") {
        if (!updater::launchAndWaitForHealth(transaction, 30000, error)) {
            std::cerr << "ERROR: " << error << '\n';
            return 9;
        }
        std::cout << "UPDATED APPLICATION HEALTHY\n";
        return 0;
    }
    if (mode == L"--apply") {
        if (!updater::applyUpdate(transaction, 30000, error)) {
            std::cerr << "ERROR: " << error << '\n';
            return 10;
        }
        std::cout << "UPDATE APPLIED AND HEALTHY\n";
        return 0;
    }
    if (mode == L"--recover") {
        if (!updater::recoverInterruptedUpdate(transaction, error)) {
            std::cerr << "ERROR: " << error << '\n';
            return 11;
        }
        std::cout << "UPDATE RECOVERY COMPLETE\n";
        return 0;
    }
    if (!updater::extractAndValidatePackage(transaction, error)) {
        std::cerr << "ERROR: " << error << '\n';
        return 5;
    }
    std::cout << "STAGED AND VALIDATED version=" << transaction.expectedVersion << '\n';
    return 0;
}
} // namespace

int wmain(int argc, wchar_t **argv)
{
    const std::wstring mode = argc >= 2 ? std::wstring(argv[1]) : std::wstring();
    if (argc == 2 && mode == L"--self-test") {
        std::cout << "GameHQUpdater " GAMEHQ_UPDATER_VERSION " ready\n";
        return 0;
    }
    if (argc != 3 || (mode != L"--dry-run" && mode != L"--stage"
                      && mode != L"--snapshot-data" && mode != L"--restore-data"
                      && mode != L"--swap" && mode != L"--wait-health"
                      && mode != L"--apply" && mode != L"--recover")) {
        std::cerr << "Usage: GameHQUpdater.exe <mode> <transaction.json>\n";
        return 1;
    }
    return runTransaction(std::filesystem::path(argv[2]), mode);
}
