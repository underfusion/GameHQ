using System;
using System.Diagnostics;
using Playnite.SDK;

namespace GameHQ.Playnite
{
    // Launches the ROOT GameHQ.exe (never app\GameHQ.exe directly) so the
    // app's own single-instance forwarding (plan item p4-3) safely handles
    // an already-running instance instead of a second full launch.
    internal static class GameHQProcessLauncher
    {
        private static readonly ILogger Logger = LogManager.GetLogger(nameof(GameHQProcessLauncher));

        public static bool TryLaunch(string rootExePath)
        {
            if (!GameHQLocator.IsValidInstall(rootExePath))
                return false;

            try
            {
                Process.Start(new ProcessStartInfo(rootExePath) { UseShellExecute = true });
                return true;
            }
            catch (Exception ex)
            {
                Logger.Warn("Failed to launch GameHQ at " + rootExePath + ": " + ex.Message);
                return false;
            }
        }
    }
}
