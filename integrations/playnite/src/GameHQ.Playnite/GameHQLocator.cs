using System;
using System.IO;
using Microsoft.Win32;

namespace GameHQ.Playnite
{
    // Finds a local GameHQ install without ever scanning the disk. Order:
    // (1) a user-configured path (Settings, lands in p5-4), (2) the
    // current-user Run registry entry GameHQ writes for its own
    // start-with-Windows option. See plan item p5-2 agent notes.
    internal static class GameHQLocator
    {
        private const string RunRegistryValueName = "GameHQ";

        public static string Locate(string configuredExePath)
        {
            if (IsValidInstall(configuredExePath))
                return configuredExePath;

            var fromRegistry = FromRunRegistry();
            return IsValidInstall(fromRegistry) ? fromRegistry : null;
        }

        // A valid install is the root launcher next to app\GameHQ.exe —
        // matches the packaged layout documented in docs/updater.md.
        public static bool IsValidInstall(string exePath)
        {
            if (string.IsNullOrWhiteSpace(exePath) || !File.Exists(exePath))
                return false;

            var root = Path.GetDirectoryName(exePath);
            if (string.IsNullOrEmpty(root))
                return false;

            return File.Exists(Path.Combine(root, "app", "GameHQ.exe"));
        }

        private static string FromRunRegistry()
        {
            try
            {
                using (var key = Registry.CurrentUser.OpenSubKey(@"Software\Microsoft\Windows\CurrentVersion\Run"))
                {
                    var raw = key != null ? key.GetValue(RunRegistryValueName) as string : null;
                    return ExtractExePath(raw);
                }
            }
            catch (Exception)
            {
                // Registry access failures must never break plugin load.
                return null;
            }
        }

        // The Run value is a full command line (e.g. "C:\...\GameHQ.exe" --minimized) —
        // take only the quoted or leading path component.
        private static string ExtractExePath(string commandLine)
        {
            if (string.IsNullOrWhiteSpace(commandLine)) return null;

            commandLine = commandLine.Trim();
            if (commandLine.StartsWith("\"", StringComparison.Ordinal))
            {
                int end = commandLine.IndexOf('"', 1);
                return end > 1 ? commandLine.Substring(1, end - 1) : null;
            }

            int space = commandLine.IndexOf(' ');
            return space > 0 ? commandLine.Substring(0, space) : commandLine;
        }
    }
}
