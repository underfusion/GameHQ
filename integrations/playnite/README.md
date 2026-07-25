# GameHQ Integration (Playnite plugin)

A small [Playnite](https://playnite.link/) add-on that connects to a locally
running [GameHQ](../../README.md) install and reports which game is running,
so GameHQ's own detection can be more reliable for windowed, launcher-based
and emulator-run games. It lives in the same repository as GameHQ as an
independently versioned subproject — see `../../START-PLAN-2026-07-06.md`
Phase 4 for the design decision.

GameHQ works completely standalone. This plugin is optional and never
required to use GameHQ; it only makes automatic game detection smarter for
Playnite users.

The plugin communicates only with the local `GameHQ.Local.v1` named pipe and
does not add telemetry. Download and security guidance is maintained in
[GameHQ Security & Privacy](../../docs/security-and-privacy.md).

## Status

Connects to a running GameHQ over `GameHQ.Local.v1`, can launch it when not
already running, forwards game start/stop/cancel events plus a full
session snapshot on every (re)connect, and has a settings page for the
GameHQ path, startup preferences and connection diagnostics. Plugin 0.4.12
has been verified in Playnite 10.56 with automatic GameHQ discovery, a
compatible v1 handshake, live diagnostics, and launch-on-game behavior.
Portable Playnite and lifecycle edge cases remain in the acceptance matrix.

## Layout

- `src/GameHQ.Playnite/` — the plugin itself (`GenericPlugin`, .NET
  Framework 4.6.2, Playnite SDK).
- `tests/GameHQ.Playnite.Tests/` — unit tests, independent of Playnite.
- `packaging/` — build, `.pext` packaging and manifest verification scripts.
- `InstallerManifest.yaml` — the manifest Playnite's add-on database and
  installer read to fetch and install this plugin.

## Building

Requires the .NET SDK on `PATH` (`dotnet --version`). From this directory:

```
dotnet build src/GameHQ.Playnite/GameHQ.Playnite.csproj
dotnet test tests/GameHQ.Playnite.Tests/GameHQ.Playnite.Tests.csproj
```

This subproject builds independently of GameHQ's own CMake build — running
`start.bat` or building the root `CMakeLists.txt` never touches it, and
building this plugin never touches GameHQ.

## Loading in Playnite (developer mode)

1. Build the plugin (above).
2. In Playnite, go to **Settings → For developers**, enable developer mode,
   and point it at `src/GameHQ.Playnite/bin/Debug/net462/`.
3. Restart Playnite.

## Releasing

Releases are tagged `playnite-vX.Y.Z` in the `underfusion/GameHQ` repository
(distinct from the app's own `vX.Y.Z` tags) and publish a single
`GameHQ_Integration_<version>.pext` asset. See `packaging/package.ps1`.

## License

The Playnite integration is under the MIT License; see its local
[`LICENSE`](LICENSE). The GameHQ core uses the same MIT terms at the repository
root.
