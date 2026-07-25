using System;
using System.Collections.Generic;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Input;
using GameHQ.Playnite.Protocol;
using Playnite.SDK;

namespace GameHQ.Playnite.Settings
{
    // Backs the settings page (plan item p5-4): connection health/testing,
    // the two startup preferences, and a copyable diagnostic summary.
    // Never duplicates any setting that lives in GameHQ itself.
    public class GameHQIntegrationSettingsViewModel : ObservableObjectBase, ISettings
    {
        private readonly GameHQPlugin _plugin;
        private readonly IPlayniteAPI _api;
        private GameHQIntegrationSettings _editingClone;

        public GameHQIntegrationSettings Settings { get; set; }

        private string _testConnectionStatusText;
        private volatile bool _testInFlight;

        private const string ReleasesUrl = "https://github.com/underfusion/GameHQ/releases";

        public GameHQIntegrationSettingsViewModel(GameHQPlugin plugin, IPlayniteAPI api)
        {
            _plugin = plugin;
            _api = api;
            Settings = plugin.Settings;

            // StateChanged fires from IntegrationClient's background reconnect
            // thread, not the UI thread — WPF bindings don't reliably pick up
            // PropertyChanged raised off-thread, so marshal onto the dispatcher.
            plugin.Client.StateChanged += (_) =>
                Application.Current.Dispatcher.BeginInvoke(new Action(() =>
                {
                    // A test in flight owns the message until it posts its own
                    // result; outside that, a state change invalidates it.
                    if (!_testInFlight) ClearTestResult();
                    RefreshConnectionStatus();
                }));
        }

        public string PluginVersionText
        {
            get { return PluginVersion(); }
        }

        public string ConnectionStatusText
        {
            get { return DescribeState(_plugin.Client.State); }
        }

        // Result of the last "Test connection" click. A local pipe
        // reconnect (see TriggerReconnect) completes in a few ms, so the
        // natural Connecting/Connected flicker on ConnectionStatusText is
        // too fast for a user to actually see — this gives an explicit
        // "Testing..." then a persistent result message instead.
        public string TestConnectionStatusText
        {
            get { return _testConnectionStatusText; }
            private set { SetValue(ref _testConnectionStatusText, value); }
        }

        public string DetectedGameHQVersionText
        {
            get { return _plugin.Client.RemoteAppVersion ?? "(not connected)"; }
        }

        // The settings page used to show an empty text box bound straight to
        // the manual override, which reads as a broken field: auto-detection
        // succeeds silently and its result is never displayed. Resolve the
        // path the same way the client does and show what actually won.
        public string ResolvedExePath
        {
            get { return GameHQLocator.Locate(Settings.ExePath); }
        }

        public bool IsGameHQFound
        {
            get { return !string.IsNullOrEmpty(ResolvedExePath); }
        }

        public string ResolvedExePathText
        {
            get { return ResolvedExePath ?? "GameHQ was not found on this PC."; }
        }

        public string LocationSourceText
        {
            get
            {
                if (!IsGameHQFound)
                    return "Install GameHQ, then use \"Locate GameHQ.exe...\" if it is still not found.";
                return GameHQLocator.IsValidInstall(Settings.ExePath)
                    ? "Selected manually"
                    : "Detected automatically";
            }
        }

        public string SelectExeButtonText
        {
            get { return IsGameHQFound ? "Change location..." : "Locate GameHQ.exe..."; }
        }

        public string ProtocolCompatibilityText
        {
            get
            {
                var selected = _plugin.Client.ProtocolSelected;
                return selected.HasValue ? "v" + selected.Value + " (compatible)" : "(unknown)";
            }
        }

        public string LastErrorText
        {
            get { return _plugin.Client.LastError ?? "(none)"; }
        }

        public ICommand SelectExeCommand
        {
            get { return new RelayCommand(SelectExe); }
        }

        public ICommand TestConnectionCommand
        {
            get { return new RelayCommand(TestConnection); }
        }

        public ICommand OpenGameHQCommand
        {
            get { return new RelayCommand(_plugin.OpenOrLaunchGameHQ); }
        }

        public ICommand OpenWebsiteCommand
        {
            get { return new RelayCommand(() => System.Diagnostics.Process.Start("https://github.com/underfusion/GameHQ")); }
        }

        public ICommand CopyDiagnosticSummaryCommand
        {
            get { return new RelayCommand(CopyDiagnosticSummary); }
        }

        public ICommand OpenFolderCommand
        {
            get { return new RelayCommand(OpenFolder); }
        }

        public ICommand DownloadGameHQCommand
        {
            get { return new RelayCommand(() => OpenUrl(ReleasesUrl)); }
        }

        public ICommand OpenPlayniteLogCommand
        {
            get { return new RelayCommand(OpenPlayniteLog); }
        }

        private void SelectExe()
        {
            var path = _api.Dialogs.SelectFile("GameHQ.exe|GameHQ.exe");
            if (string.IsNullOrEmpty(path)) return;

            if (!GameHQLocator.IsValidInstall(path))
            {
                _api.Dialogs.ShowErrorMessage(
                    "That doesn't look like a GameHQ install (expected an \"app\\GameHQ.exe\" next to it).",
                    "GameHQ Integration");
                return;
            }

            Settings.ExePath = path;
            RefreshLocation();
        }

        private void OpenFolder()
        {
            var path = ResolvedExePath;
            if (string.IsNullOrEmpty(path)) return;

            var folder = System.IO.Path.GetDirectoryName(path);
            if (!string.IsNullOrEmpty(folder))
                OpenUrl(folder);
        }

        // The integration has no log file of its own — everything it reports
        // goes into Playnite's main log, so the action is named for what it
        // actually opens. Fall back to the folder if the file isn't there yet.
        private void OpenPlayniteLog()
        {
            var configPath = _api.Paths.ConfigurationPath;
            if (string.IsNullOrEmpty(configPath))
            {
                _api.Dialogs.ShowErrorMessage("Playnite did not report a configuration path.",
                                              "GameHQ Integration");
                return;
            }

            var logFile = System.IO.Path.Combine(configPath, "playnite.log");
            OpenUrl(System.IO.File.Exists(logFile) ? logFile : configPath);
        }

        private void OpenUrl(string target)
        {
            try
            {
                System.Diagnostics.Process.Start(target);
            }
            catch (Exception ex)
            {
                _api.Dialogs.ShowErrorMessage("Could not open " + target + ": " + ex.Message,
                                              "GameHQ Integration");
            }
        }

        private void TestConnection()
        {
            TestConnectionStatusText = "Testing...";
            _testInFlight = true;
            _plugin.Client.TriggerReconnect();

            Task.Run(() =>
            {
                var deadline = DateTime.UtcNow.AddSeconds(5);
                var sawConnecting = false;
                while (DateTime.UtcNow < deadline)
                {
                    var state = _plugin.Client.State;
                    if (state == IntegrationConnectionState.Connecting) sawConnecting = true;
                    if (sawConnecting && state != IntegrationConnectionState.Connecting) break;
                    Thread.Sleep(100);
                }

                var finalState = _plugin.Client.State;
                // Deliberately version-free: the live version is one row above,
                // and repeating a snapshot of it here left a stale number on
                // screen after GameHQ was restarted on a newer build.
                var result = finalState == IntegrationConnectionState.Connected
                    ? "Connection test succeeded."
                    : "Test failed: " + (_plugin.Client.LastError ?? "could not reach GameHQ");

                Application.Current.Dispatcher.BeginInvoke(new Action(() =>
                {
                    _testInFlight = false;
                    RefreshConnectionStatus();
                    TestConnectionStatusText = result;
                }));
            });
        }

        private void CopyDiagnosticSummary()
        {
            Clipboard.SetText(BuildDiagnosticSummary());
        }

        private string BuildDiagnosticSummary()
        {
            var sb = new StringBuilder();
            sb.AppendLine("GameHQ Integration plugin: " + PluginVersion());
            sb.AppendLine("Playnite API version: " + _api.ApplicationInfo.ApplicationVersion);
            sb.AppendLine("GameHQ path: " + ResolvedExePathText + " (" + LocationSourceText + ")");
            sb.AppendLine("GameHQ version: " + DetectedGameHQVersionText);
            sb.AppendLine("Protocol: " + ProtocolCompatibilityText);
            sb.AppendLine("Connection state: " + ConnectionStatusText);
            sb.AppendLine("Last error: " + LastErrorText);
            return sb.ToString();
        }

        private static string PluginVersion()
        {
            return System.Reflection.Assembly.GetExecutingAssembly().GetName().Version.ToString();
        }

        private void RefreshConnectionStatus()
        {
            OnPropertyChanged(nameof(ConnectionStatusText));
            OnPropertyChanged(nameof(DetectedGameHQVersionText));
            OnPropertyChanged(nameof(ProtocolCompatibilityText));
            OnPropertyChanged(nameof(LastErrorText));
            RefreshLocation();
        }

        private void RefreshLocation()
        {
            OnPropertyChanged(nameof(ResolvedExePath));
            OnPropertyChanged(nameof(ResolvedExePathText));
            OnPropertyChanged(nameof(IsGameHQFound));
            OnPropertyChanged(nameof(LocationSourceText));
            OnPropertyChanged(nameof(SelectExeButtonText));
        }

        // Called whenever the live connection moves. A test result describes
        // one past moment, so it must not outlive the connection it described.
        private void ClearTestResult()
        {
            TestConnectionStatusText = null;
        }

        private static string DescribeState(IntegrationConnectionState state)
        {
            switch (state)
            {
                case IntegrationConnectionState.Connected: return "Connected";
                case IntegrationConnectionState.Connecting: return "Connecting...";
                case IntegrationConnectionState.Suspended: return "Waiting (GameHQ is updating)";
                default: return "Disconnected";
            }
        }

        public void BeginEdit()
        {
            _editingClone = new GameHQIntegrationSettings
            {
                ExePath = Settings.ExePath,
                StartWithPlaynite = Settings.StartWithPlaynite,
                StartOnGameLaunchIfNotRunning = Settings.StartOnGameLaunchIfNotRunning
            };
        }

        public void CancelEdit()
        {
            Settings = _editingClone;
        }

        public void EndEdit()
        {
            _plugin.SaveSettings(Settings);
        }

        public bool VerifySettings(out List<string> errors)
        {
            errors = new List<string>();
            if (!string.IsNullOrEmpty(Settings.ExePath) && !GameHQLocator.IsValidInstall(Settings.ExePath))
                errors.Add("The selected path is not a valid GameHQ install.");

            return errors.Count == 0;
        }
    }
}
