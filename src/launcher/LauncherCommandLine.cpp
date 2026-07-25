#include "launcher/LauncherCommandLine.h"

namespace
{
bool isSeparator(wchar_t c)
{
    return c == L' ' || c == L'\t';
}
}

namespace launcher
{
std::size_t programNameEnd(const std::wstring& commandLine)
{
    std::size_t i = 0;
    if (i < commandLine.size() && commandLine[i] == L'"') {
        ++i;
        while (i < commandLine.size() && commandLine[i] != L'"')
            ++i;
        if (i < commandLine.size())
            ++i;   // the closing quote belongs to the program name
        return i;
    }
    while (i < commandLine.size() && !isSeparator(commandLine[i]))
        ++i;
    return i;
}

std::wstring argumentTail(const std::wstring& commandLine)
{
    return commandLine.substr(programNameEnd(commandLine));
}

std::vector<std::wstring> parseArguments(const std::wstring& commandLine)
{
    const std::wstring tail = argumentTail(commandLine);
    std::vector<std::wstring> out;
    std::wstring current;
    bool inQuotes = false;
    bool started = false;

    std::size_t i = 0;
    while (i < tail.size()) {
        const wchar_t c = tail[i];
        if (!inQuotes && isSeparator(c)) {
            if (started) {
                out.push_back(current);
                current.clear();
                started = false;
            }
            ++i;
            continue;
        }
        started = true;

        if (c == L'\\') {
            // Backslashes are only escapes immediately before a quote: 2n of
            // them produce n and leave the quote to act as a delimiter, 2n+1
            // produce n and a literal quote. Anywhere else they are literal,
            // which is why a trailing "C:\path\" survives unchanged.
            std::size_t slashes = 0;
            while (i < tail.size() && tail[i] == L'\\') {
                ++slashes;
                ++i;
            }
            if (i < tail.size() && tail[i] == L'"') {
                current.append(slashes / 2, L'\\');
                if (slashes % 2 == 1) {
                    current.push_back(L'"');
                    ++i;
                }
            } else {
                current.append(slashes, L'\\');
            }
            continue;
        }

        if (c == L'"') {
            // CommandLineToArgvW's quirk: "" inside a quoted run yields one
            // literal quote *and* ends the quoted run. Matching it matters,
            // because that is what the child process will see.
            if (inQuotes && i + 1 < tail.size() && tail[i + 1] == L'"') {
                current.push_back(L'"');
                inQuotes = false;
                i += 2;
                continue;
            }
            inQuotes = !inQuotes;
            ++i;
            continue;
        }

        current.push_back(c);
        ++i;
    }

    if (started)
        out.push_back(current);
    return out;
}

bool hasSwitch(const std::wstring& commandLine, const std::wstring& name)
{
    for (const std::wstring& argument : parseArguments(commandLine)) {
        if (argument == name)
            return true;
    }
    return false;
}

std::wstring buildChildCommandLine(const std::wstring& exePath, const std::wstring& argumentTail)
{
    if (exePath.empty())
        return {};
    // 2 for the quotes around the executable, 1 for the terminating null.
    if (exePath.size() + argumentTail.size() + 3 > kMaxCommandLineChars)
        return {};
    std::wstring out;
    out.reserve(exePath.size() + argumentTail.size() + 2);
    out.push_back(L'"');
    out.append(exePath);
    out.push_back(L'"');
    out.append(argumentTail);
    return out;
}
}
