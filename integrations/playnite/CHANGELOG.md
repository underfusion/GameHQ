# Changelog

All notable changes to the GameHQ Integration Playnite plugin are documented
here. The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and the plugin uses [Semantic Versioning](https://semver.org/). This plugin
is versioned and released independently of the main GameHQ application (see
`../../VERSION` for that) — releases are tagged `playnite-vX.Y.Z` in this
same repository.

## [0.4.3] - 2026-07-21

### Fixed

- "Test connection" still gave no visible feedback: a local pipe reconnect
  completes in a few milliseconds, so the Status field's Connecting flicker
  was too fast to actually see. The button now shows "Testing..." right
  away and, once the reconnect settles, a persistent "Test succeeded" /
  "Test failed: <reason>" result line under the button.
- The settings page never showed the plugin's own version, only GameHQ's —
  it now has a "Plugin version:" row so it's obvious whether Playnite
  picked up a newly installed build.

## [0.4.2] - 2026-07-21

### Fixed

- The plugin never appeared under Playnite's Add-ons → Extensions settings
  tree (though it showed as installed/enabled) because the constructor
  didn't set `Properties.HasSettings = true`, the flag Playnite's SDK
  requires to list a settings entry regardless of `GetSettings`/
  `GetSettingsView` being overridden.
- `extension.yaml` never declared an `Icon`, so Playnite showed the plugin
  with a generic default icon instead of the packaged `icon.png`.
- The handshake always failed with "Timeouts are not supported on this
  stream" because `PipeStream.ReadTimeout` is unconditionally unsupported
  in .NET — Test Connection could never succeed even with GameHQ running.
  The handshake deadline is now enforced by racing the read against a
  timer instead of setting `ReadTimeout`.
- "Test connection" did nothing when already connected — it only woke the
  loop's backoff wait, which isn't reached while a connection is active,
  so clicking it gave no visible feedback. It now also drops the active
  pipe to force an immediate real reconnect, which is visible in the
  Status field as it cycles through Connecting.
- Even after the reconnect fix above, Test connection still appeared to
  do nothing: `StateChanged` fires from the background reconnect thread,
  and WPF doesn't reliably apply property-change notifications raised
  off the UI thread. The settings page's `StateChanged` handler now
  marshals onto the UI dispatcher before refreshing the bound fields.

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
