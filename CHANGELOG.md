# Changelog

All notable public releases of GameHQ are documented here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and the project uses
[Semantic Versioning](https://semver.org/).

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
