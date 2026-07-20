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

## Status

Connects to a running GameHQ over `GameHQ.Local.v1` and can launch it when
not already running. Forwarding game lifecycle events (start/stop) and the
settings page are not implemented yet. See `CHANGELOG.md` for what has
shipped.

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

MIT, same as GameHQ — see `../../LICENSE`.
