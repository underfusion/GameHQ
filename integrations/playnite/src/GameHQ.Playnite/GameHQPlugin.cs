using System;
using Playnite.SDK;
using Playnite.SDK.Plugins;

namespace GameHQ.Playnite
{
    // Entry point Playnite loads via extension.yaml. Kept deliberately thin:
    // the pipe client, locator/launcher and lifecycle forwarding land in
    // follow-up plan items (p5-2, p5-3) — this scaffold only proves the
    // plugin loads and never blocks Playnite.
    public class GameHQPlugin : GenericPlugin
    {
        // Stable per p1-1's reserved identifiers; must never change once published.
        public override Guid Id { get; } = new Guid("6f2b6a0a-6e0a-4b8e-9b7b-3a7d7c8b6a1d");

        public GameHQPlugin(IPlayniteAPI api) : base(api)
        {
        }
    }
}
