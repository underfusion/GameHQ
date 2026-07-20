#include "updater/UpdaterTransaction.h"
#include "updater/UpdaterStaging.h"
#include "updater/UpdaterDataSnapshot.h"
#include "updater/UpdaterSwap.h"
#include "updater/UpdaterHealth.h"
#include "updater/UpdaterRecovery.h"
#include "core/UpdaterHandshake.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <windows.h>

namespace
{
struct HandleCloser { void operator()(void *handle) const { if (handle) CloseHandle(handle); } };
using UniqueHandle = std::unique_ptr<void, HandleCloser>;

// The helper usually runs detached, so cout/cerr are invisible; mirror every
// outcome into .update/updater.log. Never opened for --dry-run, which
// promises not to write anything.
std::ofstream g_log;

void openLog(const std::filesystem::path &transactionPath)
{
    g_log.open(transactionPath.parent_path() / L"updater.log",
               std::ios::app | std::ios::binary);
}

std::string timestamp()
{
    SYSTEMTIME st{};
    GetLocalTime(&st);
    char buffer[24];
    std::snprintf(buffer, sizeof(buffer), "%04u-%02u-%02u %02u:%02u:%02u",
                  st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    return buffer;
}

void note(const std::string &line)
{
    std::cout << line << '\n';
    if (g_log) {
        g_log << timestamp() << ' ' << line << '\n';
        g_log.flush();
    }
}

int failWith(const std::string &error, int code)
{
    std::cerr << "ERROR: " << error << '\n';
    if (g_log) {
        g_log << timestamp() << " ERROR: " << error << " (exit " << code << ")\n";
        g_log.flush();
    }
    return code;
}

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
    if (mode != L"--dry-run")
        openLog(transactionPath);
    UniqueHandle activeMutex(CreateMutexW(nullptr, TRUE, L"Local\\GameHQUpdaterActive"));
    if (!activeMutex || GetLastError() == ERROR_ALREADY_EXISTS)
        return failWith("another updater helper is already active", 4);
    const std::wstring mutexName = mutexNameFor(transactionPath);
    UniqueHandle mutex(CreateMutexW(nullptr, TRUE, mutexName.c_str()));
    if (!mutex)
        return failWith("could not create updater transaction mutex", 3);
    if (GetLastError() == ERROR_ALREADY_EXISTS)
        return failWith("another updater is already processing this transaction", 4);

    updater::Transaction transaction;
    std::string error;
    if (!updater::loadAndValidateTransaction(transactionPath, transaction, error))
        return failWith(error, 2);
    if (mode != L"--dry-run")
        note("GameHQUpdater " GAMEHQ_UPDATER_VERSION " starting mode "
             + std::string(mode.begin(), mode.end()) + " for version "
             + transaction.expectedVersion);
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
        if (!updater::createDataSnapshot(transaction.dataDir, transaction.dataSnapshotDir, error))
            return failWith(error, 6);
        note("DATA SNAPSHOT READY");
        return 0;
    }
    if (mode == L"--restore-data") {
        if (!updater::restoreDataSnapshot(transaction.dataDir, transaction.dataSnapshotDir, error))
            return failWith(error, 7);
        note("DATA SNAPSHOT RESTORED");
        return 0;
    }
    if (mode == L"--swap") {
        if (!updater::swapProgramFiles(transaction, error))
            return failWith(error, 8);
        note("PROGRAM FILES SWAPPED");
        return 0;
    }
    if (mode == L"--wait-health") {
        if (!updater::launchAndWaitForHealth(transaction, 30000, error))
            return failWith(error, 9);
        note("UPDATED APPLICATION HEALTHY");
        return 0;
    }
    if (mode == L"--apply") {
        // Handshake with the caller (docs/updater.md): confirm the transaction
        // validated so the app may exit, then wait for that process to be gone
        // before any file is touched. No mutation has happened yet, so a
        // timeout aborts without needing recovery.
        UniqueHandle ready(CreateEventW(nullptr, TRUE, FALSE,
                                        handshake::readyEventNameFor(transactionPath).c_str()));
        if (ready)
            SetEvent(ready.get());
        if (transaction.callerPid > 0) {
            UniqueHandle caller(OpenProcess(SYNCHRONIZE, FALSE,
                                            static_cast<DWORD>(transaction.callerPid)));
            if (caller && WaitForSingleObject(caller.get(), 60000) == WAIT_TIMEOUT)
                return failWith("the running application did not exit before the update", 12);
        }
        if (!updater::applyUpdate(transaction, 30000, error))
            return failWith(error, 10);
        note("UPDATE APPLIED AND HEALTHY");
        return 0;
    }
    if (mode == L"--recover") {
        if (!updater::recoverInterruptedUpdate(transaction, error))
            return failWith(error, 11);
        note("UPDATE RECOVERY COMPLETE");
        return 0;
    }
    if (!updater::extractAndValidatePackage(transaction, error))
        return failWith(error, 5);
    note("STAGED AND VALIDATED version=" + transaction.expectedVersion);
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
