namespace GameHQ.Playnite.Settings
{
    // Persisted via Plugin.SavePluginSettings/LoadPluginSettings (Playnite's
    // own JSON store under the plugin's data folder) — see plan item p5-4.
    public class GameHQIntegrationSettings : ObservableObjectBase
    {
        private string _exePath = string.Empty;
        private bool _startWithPlaynite = true;
        private bool _startOnGameLaunchIfNotRunning = true;

        public string ExePath
        {
            get { return _exePath; }
            set { SetValue(ref _exePath, value); }
        }

        public bool StartWithPlaynite
        {
            get { return _startWithPlaynite; }
            set { SetValue(ref _startWithPlaynite, value); }
        }

        public bool StartOnGameLaunchIfNotRunning
        {
            get { return _startOnGameLaunchIfNotRunning; }
            set { SetValue(ref _startOnGameLaunchIfNotRunning, value); }
        }
    }
}
