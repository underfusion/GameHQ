# Changelog

All notable changes to the GameHQ Integration Playnite plugin are documented
here. The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and the plugin uses [Semantic Versioning](https://semver.org/). This plugin
is versioned and released independently of the main GameHQ application (see
`../../VERSION` for that) — releases are tagged `playnite-vX.Y.Z` in this
same repository.

## [0.2.0] - 2026-07-20

### Added

- `GameHQ.Local.v1` pipe client (framing, handshake, bounded outgoing queue,
  reconnect with exponential backoff, `app.maintenance` awareness) under
  `Protocol/`.
- `GameHQLocator` (configured path, then the current-user Run registry
  entry) and `GameHQProcessLauncher` (launches the root `GameHQ.exe`).
- The plugin now starts the client on load and makes one best-effort launch
  attempt on `OnApplicationStarted` if GameHQ isn't already reachable.
  Game lifecycle forwarding still lands in a follow-up release (p5-3).

## [0.1.0] - 2026-07-20

### Added

- Initial subproject scaffold: buildable `GenericPlugin` skeleton targeting
  the Playnite 10 SDK, its own `.csproj`, packaging scripts and installer
  manifest. No user-facing behavior yet — connecting to GameHQ, forwarding
  game lifecycle events and the settings page land in follow-up releases.
