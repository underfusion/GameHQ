using System;
using System.IO;
using Microsoft.Win32;

namespace GameHQ.Playnite
{
    // Finds a local GameHQ install without ever scanning the disk. A live pipe
    // connection is checked by GameHQPlugin before it calls this locator. The
    // remaining order is manual path, installer contract, App Paths, Run, then
    // the standard per-user install directory. The public contract is frozen
    // in docs/packaging.md and packaging/distribution-identity.psd1.
    internal static class GameHQLocator
    {
        private const string ProductRegistryKey = @"Software\underfusion\GameHQ";
        private const string InstallLocationValueName = "InstallLocation";
        private const string AppPathRegistryKey = @"Software\Microsoft\Windows\CurrentVersion\App Paths\GameHQ.exe";
        private const string RunRegistryKey = @"Software\Microsoft\Windows\CurrentVersion\Run";
        private const string RunRegistryValueName = "GameHQ";
        private const string RootExeName = "GameHQ.exe";

        public static string Locate(string configuredExePath)
        {
            var candidate = FirstValid(configuredExePath);
            if (candidate != null) return candidate;

            candidate = FirstValid(FromInstallLocationRegistry());
            if (candidate != null) return candidate;

            candidate = FirstValid(FromAppPathsRegistry());
            if (candidate != null) return candidate;

            candidate = FirstValid(FromRunRegistry());
            if (candidate != null) return candidate;

            return FirstValid(FromDefaultInstallLocation());
        }

        internal static string LocateFromCandidates(
            string configuredExePath,
            string installLocationExePath,
            string appPathExePath,
            string runExePath,
            string defaultExePath)
        {
            return FirstValid(
                configuredExePath,
                installLocationExePath,
                appPathExePath,
                runExePath,
                defaultExePath);
        }

        // A valid install is the root launcher next to app\GameHQ.exe —
        // matches the packaged layout documented in docs/updater.md.
        public static bool IsValidInstall(string exePath)
        {
            if (string.IsNullOrWhiteSpace(exePath) || !File.Exists(exePath))
                return false;

            if (!string.Equals(Path.GetFileName(exePath), RootExeName, StringComparison.OrdinalIgnoreCase))
                return false;

            var root = Path.GetDirectoryName(exePath);
            if (string.IsNullOrEmpty(root))
                return false;

            return File.Exists(Path.Combine(root, "app", "GameHQ.exe"));
        }

        private static string FirstValid(params string[] candidates)
        {
            foreach (var candidate in candidates)
            {
                if (!IsValidInstall(candidate)) continue;

                try { return Path.GetFullPath(candidate); }
                catch (Exception) { return candidate; }
            }

            return null;
        }

        private static string FromInstallLocationRegistry()
        {
            try
            {
                using (var key = Registry.CurrentUser.OpenSubKey(ProductRegistryKey))
                {
                    var root = key != null ? key.GetValue(InstallLocationValueName) as string : null;
                    if (string.IsNullOrWhiteSpace(root)) return null;
                    return Path.Combine(root.Trim(), RootExeName);
                }
            }
            catch (Exception)
            {
                return null;
            }
        }

        private static string FromAppPathsRegistry()
        {
            try
            {
                using (var key = Registry.CurrentUser.OpenSubKey(AppPathRegistryKey))
                {
                    var raw = key != null ? key.GetValue(null) as string : null;
                    return ExtractExePath(raw);
                }
            }
            catch (Exception)
            {
                return null;
            }
        }

        private static string FromRunRegistry()
        {
            try
            {
                using (var key = Registry.CurrentUser.OpenSubKey(RunRegistryKey))
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

        private static string FromDefaultInstallLocation()
        {
            try
            {
                var localAppData = Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData);
                return string.IsNullOrWhiteSpace(localAppData)
                    ? null
                    : Path.Combine(localAppData, "Programs", "GameHQ", RootExeName);
            }
            catch (Exception)
            {
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
