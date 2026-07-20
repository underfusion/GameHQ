using System;
using System.Reflection;
using GameHQ.Playnite.Protocol;
using Playnite.SDK;
using Playnite.SDK.Events;
using Playnite.SDK.Plugins;

namespace GameHQ.Playnite
{
    // Entry point Playnite loads via extension.yaml. The pipe client keeps
    // trying to connect/reconnect for the plugin's whole lifetime; game
    // lifecycle forwarding (OnGameStarting/Started/Stopped) lands in p5-3.
    public class GameHQPlugin : GenericPlugin
    {
        // Stable per p1-1's reserved identifiers; must never change once published.
        public override Guid Id { get; } = new Guid("6f2b6a0a-6e0a-4b8e-9b7b-3a7d7c8b6a1d");

        private readonly IntegrationClient _client;
        private readonly GameLifecycleForwarder _lifecycle;
        private bool _launchAttempted;

        public GameHQPlugin(IPlayniteAPI api) : base(api)
        {
            var version = Assembly.GetExecutingAssembly().GetName().Version.ToString();
            _client = new IntegrationClient(version);
            _lifecycle = new GameLifecycleForwarder(api, _client);
            _client.Start();
        }

        public override void OnApplicationStarted(OnApplicationStartedEventArgs args)
        {
            // One best-effort launch if GameHQ isn't reachable yet; the
            // client's own background loop keeps retrying the connection
            // regardless, so a failed launch here is never fatal.
            if (_launchAttempted || _client.State == IntegrationConnectionState.Connected) return;
            _launchAttempted = true;

            var exePath = GameHQLocator.Locate(null);
            if (exePath != null)
                GameHQProcessLauncher.TryLaunch(exePath);
        }

        public override void OnApplicationStopped(OnApplicationStoppedEventArgs args)
        {
            // Never close GameHQ here — it may be tray-resident, exporting a
            // clip, or used standalone without Playnite at all.
            _lifecycle.ApplicationStopping();
            _client.Stop();
        }

        public override void OnGameStarting(OnGameStartingEventArgs args)
        {
            _lifecycle.GameStarting(args.Game);
        }

        public override void OnGameStarted(OnGameStartedEventArgs args)
        {
            _lifecycle.GameStarted(args.Game, args.StartedProcessId);
        }

        public override void OnGameStopped(OnGameStoppedEventArgs args)
        {
            _lifecycle.GameStopped(args.Game);
        }

        public override void OnGameStartupCancelled(OnGameStartupCancelledEventArgs args)
        {
            _lifecycle.GameStartupCancelled(args.Game);
        }
    }
}
