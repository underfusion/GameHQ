# Changelog

All notable public releases of GameHQ are documented here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and the project uses
[Semantic Versioning](https://semver.org/).

## [0.5.74] - 2026-07-16

### Changed

- The logic behind selecting several captures at once -- which ones are ticked,
  where a shift-extend measures from, how "select all" flips to "deselect all"
  -- now lives in a file of its own instead of being spread through the main
  window. It was about eighty lines tangled up with window code it had nothing
  to do with, which made the one genuinely fiddly part (dragging a shift-range
  back over itself to undo it) hard to follow. Nothing about selecting works
  differently; the sounds, the controller buttons and the delete confirmation
  are all untouched.

## [0.5.73] - 2026-07-16

### Changed

- Working out which game you are playing no longer re-reads the disk every 1.5
  seconds. The app checks the foreground window on a timer, and each check was
  scanning your Steam library folder and reading the game executable's embedded
  metadata again from scratch -- work that returns the same answer every time,
  because it describes a file that is not changing. That answer is now
  remembered for as long as the same game stays in front, and looked up again
  the moment a different one takes over. The game's own window caption is still
  read fresh each time, so a title that appears late still shows up.

## [0.5.72] - 2026-07-16

### Changed

- The technical audit document now describes the code as it is today. It was
  written a week ago, before this round of cleanup, so it still listed problems
  that have since been fixed and pointed at code that has since moved. It now
  records what this round changed, and -- more usefully -- keeps an honest list
  of the work that is deliberately not done: five finished changes still waiting
  on a hands-on check, and five larger rewrites left alone because the recording
  and controller code they touch was tuned against real games and real hardware
  and cannot be re-checked without them.

## [0.5.71] - 2026-07-16

### Added

- The project has its first automated tests: 51 checks covering game-name
  handling, the settings file (defaults, overrides, resetting, and preserving
  keys written by a future version), and the rule that picks the better of two
  duplicate game names. They are opt-in (`-DGAMEHQ_BUILD_TESTS=ON`), cover pure
  logic only -- no recording, no controller, no database -- and run in under a
  second. The command is documented in `docs/dev-setup.md`.

### Changed

- Working out which game a capture belongs to from its folder now lives next to
  the rest of the game-name logic, and uses the same "Unknown Game" fallback as
  everything else instead of its own copy of the text.

## [0.5.70] - 2026-07-16

### Changed

- The two identical read-only folder boxes on the Capture settings page (the
  screenshots root and the clips root) are now one shared `SettingsPathField`
  component rather than the same twenty lines written twice. The page looks and
  behaves exactly as before.

## [0.5.69] - 2026-07-16

### Changed

- Picking a sidebar category now runs through one shared rule instead of three
  separate copies of it (desktop mouse click, desktop controller navigation, and
  the overlay). The `Game` and `Game Favourites` rows are the only special
  cases, and they are now described in exactly one place, so the three sidebars
  can no longer disagree about what a row means. Every row filters exactly as
  before.

## [0.5.68] - 2026-07-16

### Changed

- The play marker shown on video thumbnails is now one shared `VideoBadge`
  component instead of three separate copies of the same drawing in the capture
  tile, the overlay preview and the toast. The three only ever differed in size,
  which each caller still sets, so the badge looks exactly as before -- but it
  can no longer end up looking different in one place after an edit.

## [0.5.67] - 2026-07-16

### Changed

- The visual values that were still written directly into individual QML
  components -- the video play badge, the thumbnail icon buttons, the player's
  play/pause pulse, the tinted Delete/Done buttons, row hover, two font sizes
  and the all-caps letter spacing -- now come from named `Theme` tokens like
  every other value. Each literal was mapped to a token of exactly the same
  value, so nothing changes visually; the point is that these values can now be
  changed in one place.

## [0.5.66] - 2026-07-16

### Changed

- Controller and keyboard actions are now looked up in one dispatch table
  instead of a 28-branch chain of string comparisons, keeping the list of
  actions next to the `ActionCatalog` it mirrors. An action that is ever left
  out of the table now logs a warning instead of failing silently. No shortcut,
  binding, or hold-to-repeat behavior changes.

## [0.5.65] - 2026-07-16

### Changed

- The compatibility shims that adopt data from the app's former names (SavePlay,
  PlayHQ) now live in one place, a new `LegacyMigration` helper, instead of being
  spread between `Paths` and an inline block in `App::init`. The database
  hand-over is reached through the new `Paths::databasePath()`. Migration
  behavior is unchanged: legacy folders and a legacy database are still adopted
  on first start, and capture media is still never moved.

## [0.5.64] - 2026-07-16

### Changed

- The special-cased settings keys (the startup toggle and the screenshot/clip
  root pickers) moved out of `AppController::setConfig`/`resetConfig` into a
  new internal `SettingsRouter` helper, matching the existing
  `CaptureLibraryService` delegation pattern. Those two methods are now a plain
  read of the router's outcome. Behavior and the QML API are unchanged.

## [0.5.63] - 2026-07-16

### Changed

- Every `config.json` key is now spelled once, in the new
  `src/config/ConfigKeys.h` registry, and referenced from C++ through a
  constant instead of a repeated string literal. A mistyped key is now a
  compile error rather than a silent fall back to the default value. QML keeps
  using string literals, since it cannot see the constants.
- The settings "Restore defaults" taxonomy (which config groups each page
  owns) moved out of `AppController` into `src/config/SettingsCategories.h`,
  next to the key registry.

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
