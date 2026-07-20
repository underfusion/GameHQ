# Changelog

All notable changes to the GameHQ Integration Playnite plugin are documented
here. The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and the plugin uses [Semantic Versioning](https://semver.org/). This plugin
is versioned and released independently of the main GameHQ application (see
`../../VERSION` for that) — releases are tagged `playnite-vX.Y.Z` in this
same repository.

## [0.4.0] - 2026-07-20

### Added

- Settings page (Connection / Startup / Support sections): configurable
  GameHQ path with validation, live connection status, detected GameHQ
  version and protocol, a "Test connection" action, the two startup
  preferences, "Open GameHQ", "Website" and "Copy diagnostic summary".
- Main-menu command "Open GameHQ" (focuses a running instance or launches
  it) — intentionally the only main-menu command; no screenshot/replay
  commands, since opening a menu defocuses the game.
- `IntegrationClient` now tracks the remote app version, negotiated
  protocol and last error for the settings page, and exposes
  `TriggerReconnect()` for the "Test connection" action.

### Known gaps

- "Open plugin log" from the Support section is not implemented — Playnite
  doesn't expose a log path via the SDK and guessing at one risked being
  wrong; deferred to a follow-up once a real Playnite install is available
  to confirm the right target.

## [0.3.0] - 2026-07-20

### Added

- `GameLifecycleForwarder`: tracks active Playnite game sessions and sends
  `playnite.game.starting/started/stopped/startup_cancelled` and
  `playnite.application.started/stopping` over the pipe client.
- `playnite.state.sync` is sent with the current session snapshot after
  every successful connection, including reconnects, so a missed
  disconnect never leaves a phantom "game running" state on the app side.
- Playnite exiting never closes GameHQ — it may be tray-resident or used
  standalone.

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
