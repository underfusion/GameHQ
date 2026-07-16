# Changelog

All notable public releases of GameHQ are documented here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and the project uses
[Semantic Versioning](https://semver.org/).

## [0.5.62] - 2026-07-16

### Changed

- Library scans now read the capture index once instead of querying the
  database twice per file on disk. A scan of a library with N media files
  previously issued up to 2N queries during the directory walk; it now issues
  a single up-front `SELECT` and diffs in memory. Registered captures, their
  game assignment, and thumbnail backfill are unchanged.

## [0.5.61] - 2026-07-16

### Changed

- The two heavy one-time startup repairs — the O(n²) duplicate display-name
  collapse and the full `gamehq.log` metadata rescan — are now gated behind a
  completion sentinel (`internal.repairs_v1_done` in the `settings` table).
  They run once after an upgrade and are skipped on every subsequent launch,
  cutting startup cost. The sentinel is written inside the same repair
  transaction, so it only sticks if the repairs commit successfully.

## [0.5.60] - 2026-07-16

### Fixed

- The database startup repair pass (legacy brand paths, duplicate captures,
  moved-path renormalization, game metadata) now runs inside a single
  transaction, so an interrupted launch can no longer leave the library
  half-repaired.

## [0.5.59] - 2026-07-16

### Fixed

- Replay audio timestamps now use an overflow-safe clock conversion shared
  with the video pipeline (the previous audio-only formula could overflow
  after about a day of system uptime and skew A/V sync).
- The replay capture bring-up no longer leaks a d3d11.dll module reference on
  every buffer arm.
- Oversized system-audio packets that cannot fit the capture buffer are now
  logged instead of being dropped silently.

### Changed

- Capture subsystem helpers (HRESULT logging, clock conversion, stale-cache
  threshold, collision-safe file naming) consolidated into one shared header.

## [0.5.58] - 2026-07-16

### Removed

- Unused input-layer code: the superseded `TapHoldDetector` class and the
  legacy integer controller signals that had no remaining consumers. No
  behavior change; controller dispatch is unaffected.

## [0.5.57] - 2026-07-16

### Documentation

- Completed a full-codebase technical audit and recorded the Refactor Wave 2
  execution plan in the internal planning docs (dead-code removal, startup-cost
  fixes, config key centralization, theme-token cleanup, first automated test
  target). No code changes.

## [0.5.56] - 2026-07-15

### Changed

- Settings → General: the "Start minimized" toggle is now labeled
  "Launch minimized" with a clearer description.

## [0.5.55] - 2026-07-13

First public release.

### Added

- Native Windows capture gallery with game grouping, filters, favorites,
  thumbnails, bulk selection, lightbox image viewing, and video playback.
- Screenshot capture in PNG or JPEG with configurable quality, folders,
  notifications, and sounds.
- Rolling H.264 replay buffer with configurable duration, quality, frame rate,
  resolution, system audio, and MP4 export.
- Controller-driven in-game overlay with focus restoration and capture actions.
- Configurable controller, keyboard, and extra mouse-button bindings with two
  slots per action, gesture support, conflict handling, profiles, and resets.
- Separate managed screenshot and clip locations plus read-only watched folders.
- Portable mode, tray behavior, startup options, diagnostics, and scoped restore
  controls.

### Security and privacy

- No account, telemetry, cloud dependency, game-process injection, or background
  Windows service.
- User data remains local in portable storage or the current Windows profile.

[0.5.55]: https://github.com/underfusion/GameHQ/releases/tag/v0.5.55
