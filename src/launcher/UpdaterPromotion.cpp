#include "launcher/UpdaterPromotion.h"
#include <vector>
#include <windows.h>

namespace launcher
{
bool promotePendingUpdater(const std::filesystem::path &root)
{
    const auto pending = root / L"GameHQUpdater.pending.exe";
    if (!std::filesystem::is_regular_file(pending))
        return true;
    HANDLE active = OpenMutexW(SYNCHRONIZE, FALSE, L"Local\\GameHQUpdaterActive");
    if (active) {
        CloseHandle(active);
        return true; // the old helper still owns its executable; retry next launch
    }

    std::wstring command = L"\"" + pending.wstring() + L"\" --self-test";
    std::vector<wchar_t> mutableCommand(command.begin(), command.end());
    mutableCommand.push_back(L'\0');
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    bool valid = CreateProcessW(pending.c_str(), mutableCommand.data(), nullptr, nullptr,
                                FALSE, CREATE_NO_WINDOW, nullptr, root.c_str(),
                                &startup, &process);
    if (valid) {
        CloseHandle(process.hThread);
        const DWORD wait = WaitForSingleObject(process.hProcess, 5000);
        DWORD exitCode = 1;
        valid = wait == WAIT_OBJECT_0 && GetExitCodeProcess(process.hProcess, &exitCode)
            && exitCode == 0;
        if (wait == WAIT_TIMEOUT)
            TerminateProcess(process.hProcess, 1);
        CloseHandle(process.hProcess);
    }
    if (!valid) {
        std::error_code ignored;
        std::filesystem::remove(pending, ignored);
        return false;
    }
    const auto current = root / L"GameHQUpdater.exe";
    if (!MoveFileExW(pending.c_str(), current.c_str(),
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
        return false;
    return true;
}
}
