#pragma once

#include <string>
#include <vector>

// Command-line handling for the package launcher, kept free of <windows.h> so
// it can be unit-tested. The launcher forwards whatever it was given to
// app\GameHQ.exe, so getting the quoting rules wrong silently changes the
// arguments a user typed — and the old fixed-size buffers could overflow long
// before that mattered.
namespace launcher
{
// A Windows command line is at most 32767 characters including the terminating
// null (CreateProcessW's documented limit for lpCommandLine).
inline constexpr std::size_t kMaxCommandLineChars = 32767;

// Offset of the first character after the program name. Windows parses that
// first field with its own rule: a quoted name ends at the next quote and
// backslashes are never escapes there.
std::size_t programNameEnd(const std::wstring& commandLine);

// Everything after the program name, verbatim — leading separators included.
// Forwarding the tail unchanged is what keeps quotes, embedded spaces and
// trailing backslashes byte-identical for the child; re-quoting a parsed
// vector would not.
std::wstring argumentTail(const std::wstring& commandLine);

// The arguments after the program name, tokenized by the same rules
// CommandLineToArgvW uses. Only for inspecting arguments, never for rebuilding
// the command line.
std::vector<std::wstring> parseArguments(const std::wstring& commandLine);

// True when `name` appears as a whole argument. A substring search would also
// fire on a file path that merely contains the switch text.
bool hasSwitch(const std::wstring& commandLine, const std::wstring& name);

// "<exePath>" followed by `argumentTail`. Empty when the result would not fit
// in a Windows command line, so the caller refuses instead of truncating.
std::wstring buildChildCommandLine(const std::wstring& exePath,
                                   const std::wstring& argumentTail);
}
