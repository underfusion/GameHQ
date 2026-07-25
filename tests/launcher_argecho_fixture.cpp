// Stands in for app\GameHQ.exe in the launcher pass-through test: it writes
// the arguments it actually received, as Windows parsed them, to the file named
// by GAMEHQ_ARGECHO_OUT. The destination comes from the environment so the
// arguments under test reach it completely untouched.
#include <windows.h>
#include <shellapi.h>

#include <cstdio>
#include <string>
#include <vector>

int wmain()
{
    std::vector<wchar_t> out(32768);
    const DWORD length = GetEnvironmentVariableW(L"GAMEHQ_ARGECHO_OUT", out.data(),
                                                 static_cast<DWORD>(out.size()));
    if (length == 0 || length >= out.size())
        return 2;

    int count = 0;
    wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &count);
    if (!argv)
        return 3;

    FILE* file = _wfopen(out.data(), L"wb");
    if (!file) {
        LocalFree(argv);
        return 4;
    }
    // UTF-8, one argument per line, argv[0] included so the test can also check
    // the child was told the right executable.
    for (int i = 0; i < count; ++i) {
        const std::wstring argument(argv[i]);
        const int bytes = WideCharToMultiByte(CP_UTF8, 0, argument.c_str(),
                                              static_cast<int>(argument.size()),
                                              nullptr, 0, nullptr, nullptr);
        std::string utf8(static_cast<std::size_t>(bytes), '\0');
        if (bytes > 0) {
            WideCharToMultiByte(CP_UTF8, 0, argument.c_str(), static_cast<int>(argument.size()),
                                utf8.data(), bytes, nullptr, nullptr);
        }
        std::fwrite(utf8.data(), 1, utf8.size(), file);
        std::fputc('\n', file);
    }
    std::fclose(file);
    LocalFree(argv);
    return 0;
}
