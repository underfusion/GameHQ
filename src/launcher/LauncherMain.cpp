// GameHQ package launcher — the only exe at the clean root (docs/packaging.md).
// Starts app\GameHQ.exe (where all Qt/ffmpeg DLLs live) so the root folder stays
// clean: launcher + README + data folders. Pure Win32, statically linked, no Qt.
#include <windows.h>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    wchar_t root[MAX_PATH];
    const DWORD len = GetModuleFileNameW(nullptr, root, MAX_PATH);
    if (len == 0 || len >= MAX_PATH)
        return 1;
    wchar_t* slash = wcsrchr(root, L'\\');
    if (!slash)
        return 1;
    *slash = L'\0';

    wchar_t exe[MAX_PATH];
    wsprintfW(exe, L"%s\\app\\GameHQ.exe", root);
    if (GetFileAttributesW(exe) == INVALID_FILE_ATTRIBUTES) {
        MessageBoxW(nullptr, L"app\\GameHQ.exe not found next to the launcher.",
                    L"GameHQ", MB_ICONERROR);
        return 1;
    }

    // Pass the original arguments through; quote the child exe path.
    wchar_t cmdLine[MAX_PATH * 2];
    const wchar_t* args = GetCommandLineW();
    if (*args == L'"') {
        ++args;
        while (*args && *args != L'"') ++args;
        if (*args == L'"') ++args;
    } else {
        while (*args && *args != L' ' && *args != L'\t') ++args;
    }
    wsprintfW(cmdLine, L"\"%s\"%s", exe, args);

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    // Working dir = package root so portable.flag/data resolution stays obvious.
    if (!CreateProcessW(exe, cmdLine, nullptr, nullptr, FALSE, 0, nullptr, root,
                        &si, &pi)) {
        MessageBoxW(nullptr, L"Failed to start app\\GameHQ.exe.", L"GameHQ",
                    MB_ICONERROR);
        return 1;
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return 0;
}
