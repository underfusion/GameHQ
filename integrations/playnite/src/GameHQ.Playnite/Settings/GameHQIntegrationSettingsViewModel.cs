using System;
using System.Collections.Generic;
using System.Text;
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

        public GameHQIntegrationSettingsViewModel(GameHQPlugin plugin, IPlayniteAPI api)
        {
            _plugin = plugin;
            _api = api;
            Settings = plugin.Settings;

            plugin.Client.StateChanged += (_) => RefreshConnectionStatus();
        }

        public string ConnectionStatusText
        {
            get { return DescribeState(_plugin.Client.State); }
        }

        public string DetectedGameHQVersionText
        {
            get { return _plugin.Client.RemoteAppVersion ?? "(not connected)"; }
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
        }

        private void TestConnection()
        {
            _plugin.Client.TriggerReconnect();
            RefreshConnectionStatus();
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
            sb.AppendLine("GameHQ path: " + (string.IsNullOrEmpty(Settings.ExePath) ? "(auto-detected)" : Settings.ExePath));
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
