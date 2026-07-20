# Changelog

All notable public releases of GameHQ are documented here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and the project uses
[Semantic Versioning](https://semver.org/).

## [0.6.12] - 2026-07-20

### Added

- Install-and-restart controls: the update banner and About page now cover the
  full install flow (ready, preparing, installing) with state-specific actions,
  and a failed check offers "Check again" instead of a misleading retry.
- Updater READY handshake: the app only exits after the helper confirms it
  validated the transaction, and the helper waits for the old process to fully
  exit before touching any file.
- Persistent `.update/updater.log` recording every helper outcome, retained
  across the post-update cleanup for diagnostics.

### Fixed

- `UpdateService` double ownership: the service is no longer parented to the
  QML engine while also held by the application, removing a shutdown
  double-delete.
- Data-restore rollback now reverses only the operations it performed instead
  of deleting every known state file, so an aborted restore can no longer
  discard an untouched database.
- A release whose assets are still uploading is no longer offered, and its
  ETag is not cached (previously the check could stick on 304 and never see
  the finished assets).
- A failed health-token release from job supervision now rolls back instead of
  reporting success while the new process is killed on helper exit.
- Screenshot encoding now finishes before shutdown, preventing a crash when
  quitting during a background encode.
- Integration clients rejected for malformed frames free their connection slot
  immediately, and install-directory game matching no longer accepts
  executables from a different drive.
- Completed updates also clean the downloaded package and stale transaction
  file; the About page License link points at the real repository file.

## [0.6.11] - 2026-07-20

### Added

- Same-user `GameHQ.Local.v1` integration channel with bounded 64 KiB framing,
  strict UTF-8/JSON/type validation, handshake negotiation, lifecycle snapshots,
  structured replies, disconnect expiry, and hostile-input tests.
- Friendly second-instance forwarding for window activation and gallery opening,
  with short connection/reply bounds and the existing lock as final authority.
- Playnite identity hints for foreground detection. Exact or descendant process
  evidence can recognize windowed games; names and directory hints never weaken
  existing capture-safety gates.
- Durable update-maintenance suppression across the app, launcher, helper and
  local protocol, including terminal cleanup and five-minute stale recovery.

## [0.6.10] - 2026-07-20

### Added

- Safe update-package download and cancellation: HTTPS-only redirects, bounded
  streaming into install-local `.partial` files, atomic publication, stale
  partial cleanup, exact release-size enforcement, and visible progress.
- Strict `.sha256` parsing and local SHA-256 verification. Malformed checksum
  files, mismatched package names, truncated downloads, and corrupted packages
  are deleted before the update can enter `ReadyToInstall`.
- Focused updater tests covering accepted checksum formats and rejection of
  malformed, mismatched, and corrupted packages.
- Static `GameHQUpdater.exe` Stage 1 foundation with a strict transaction
  schema, canonical package-root path containment, a per-transaction mutex,
  and `--dry-run` output listing every planned operation without file writes.
- Transaction tests prove a valid dry run leaves staging and backup absent,
  while an escaped backup path is rejected before anything is created.
- Hardened Stage 2 ZIP staging with pinned miniz 3.1.2: the helper re-hashes
  the package, enforces file/count/size/method and positive-path allowlists,
  rejects traversal, links, user-data paths and invalid manifests/layouts,
  and removes staging after every rejection without touching live files.
- Update quiescence barrier: new screenshot/replay requests are blocked during
  preparation, in-flight writes and clip exports finish naturally, replay
  auto-arming stops, configuration is flushed, and a 30-second timeout cancels
  the update instead of cancelling or killing capture work.
- Helper-side data snapshot and restoration for config plus SQLite DB/WAL/SHM.
  The manifest preserves originally absent sidecars, restoration has its own
  rollback path, and automated mutation/restore coverage proves `Captures/`
  remains byte-identical.
- Allowlisted program-file swap with bounded Windows lock retries and reverse-
  order rollback. Tests cover a successful swap preserving `portable.flag`
  and a deliberately locked executable aborting with old and staged files intact.
- Healthy-start validation keeps capture hooks disarmed until the upgraded app
  survives seven seconds with its database, services, QML and event loop active;
  the helper accepts only the matching version token and times out safely.
- Durable update phase and swap journals make interrupted replacement
  deterministic. Failed health checks stop the supervised process tree, restore
  program and data state together, and restart the previous version.
- Updater helpers now advertise and enforce their protocol version. A packaged
  replacement is staged under a pending name, self-tested, and promoted by a
  later launcher only after the previous helper has exited.
- End-to-end transaction coverage exercises abandoned staging cleanup,
  extraction, snapshot, swap and healthy launch while proving settings,
  captures and `portable.flag` remain byte-identical.
- Automatic-update preflight now rejects unpackaged, unwritable, network,
  unsupported-filesystem, overlong, low-space and active-transaction targets;
  packaged autostart always uses the recovery-aware root launcher.
- A successful health-validated update records and shows a one-time greeting
  with a version-specific What's New link, deferred until the desktop window
  is visible when GameHQ starts minimized.
- Release packaging now emits separate portable and update-only ZIPs plus a
  SHA-256 file. A mandatory validator rejects missing, forbidden, mismatched or
  untested artifacts before publication.
- The final install action now revalidates the GitHub release, writes a helper-
  validated transaction, launches the updater, and exits only after successful
  handoff; withdrawn, superseded or changed releases are refused.

## [0.6.9] - 2026-07-20

### Added

- Automatic update-check policy and a non-modal update banner (Phase 1 of
  the updater plan, `docs/updater.md`). `App::init()` primes `UpdateService`
  from config, runs the first automatic check 15-30s after startup, and
  re-checks at most once every 24 hours via an hourly gate timer; manual
  checks always bypass that cache. New config keys: `updates.check_automatically`
  (default on), `updates.skipped_version`, and internal persistence keys
  `internal.updates.etag` / `internal.updates.last_check_utc` (survive
  "Restore all settings" like other `internal.*` keys).
- `UpdateBanner.qml`: shown only in the desktop gallery window (never over
  the pad overlay or a running game) when a newer stable release exists —
  version, publish date, size, and release notes rendered as plain text
  (no Markdown/HTML interpretation), with "View on GitHub" (the standalone
  fallback until download/install lands in Phase 2), "Skip this version",
  and "Not now".
- Settings → About "Updates" section now reflects real state (checking /
  up to date / update available / last-checked time) with a working
  "Check now" button and a "Check automatically" toggle, replacing the
  placeholder shipped in 0.6.6.

## [0.6.8] - 2026-07-20

### Added

- Release lookup and update-check service (`src/updates/`): `ReleaseInfo`,
  `GitHubReleaseSource` (queries the GitHub releases API with ETag caching,
  a bounded retry, and confirmed-rate-limit detection via
  `x-ratelimit-remaining`/`x-ratelimit-reset`), and `UpdateService`, a state
  machine (`Idle`/`Checking`/`UpToDate`/`UpdateAvailable`/…/`Failed`)
  exposed to QML as the `updates` context object. Only exact-named
  `GameHQ-<version>-win64-update.zip` (+ `.sha256`) assets are selected;
  drafts, prereleases, and releases not newer than the installed version are
  rejected. A failed or rate-limited check never regresses a known-good
  result. Download/install commands are declared but not yet implemented —
  that lands with the safe updater helper.

## [0.6.7] - 2026-07-20

### Added

- Linked the Qt Network module (needed by the upcoming update checker and
  the local integration channel).
- `VersionNumber` (`src/updates/`): strict `major.minor.patch` parsing and
  numeric comparison for release version strings, with an optional leading
  `v`/`V` and no prerelease/build-metadata suffix accepted. Versions are
  never compared as strings. Covered by `tst_versionnumber` (valid/invalid
  parsing, numeric ordering, `v`-prefix equivalence).

## [0.6.6] - 2026-07-20

### Added

- About settings page (`Settings → About`): application logo, name, version,
  storage mode, an Updates placeholder for the upcoming update checker, and
  project links (website, GitHub, releases, issues, license) plus a GitHub
  star call-to-action, all reading from `Brand.qml`. Version and Storage mode
  rows moved here from the Advanced page.

## [0.6.5] - 2026-07-20

### Changed

- Centralized project links (website, repository, releases, issues) in the
  `Brand.qml` singleton instead of hard-coding them per page. The GitHub
  link in the Help view now reads from `Brand.repositoryUrl`.

## [0.6.4] - 2026-07-20

### Added

- Design documentation for the upcoming update system and local integration
  channel: [`docs/updater.md`](docs/updater.md) (path ownership contract,
  the nine-stage helper flow, authenticity limits) and
  [`docs/integration-protocol.md`](docs/integration-protocol.md) (the
  `GameHQ.Local.v1` named-pipe protocol used by the future Playnite
  companion plugin).
- Reserved the canonical identifiers for that work (pipe name, release asset
  naming, plugin repo name, add-on identifiers) so none are invented ad hoc
  during implementation.
- Added the `0.6.x — Distribution & Integration Foundation` milestone to
  [`docs/roadmap.md`](docs/roadmap.md).

## [0.6.3] - 2026-07-18

### Added

- Hidden-controller detection: the app now cross-checks the Windows PnP
  device tree against Raw Input on every device change. A supported Sony/DS4
  pad that Windows sees but applications cannot (the signature of the
  HidHide filter driver installed alongside DSX / DS4Windows / reWASD) is
  reported in Settings → Input with a clear explanation instead of the app
  silently detecting nothing.
- One-click remedy for HidHide-hidden pads: a "Fix automatically" button in
  Settings → Input relaunches GameHQ elevated (single UAC prompt) and adds
  the app to HidHide's application allow-list through the driver's documented
  control interface — no third-party tools, nothing installed or removed;
  DSX setups keep working.

### Changed

- The overlay sidebar now centers the GameHQ brand lockup and uses the same
  larger icon, bright label, and semibold typography as the desktop sidebar.
- The original navy theme is now named **Blue** in Settings; its internal
  `dark` key remains supported so existing preferences keep working.

### Fixed

- Fresh installations now actually start with Obsidian. The QML fallback and
  Settings reset already selected Obsidian, but the C++ configuration defaults
  still returned `dark` and overrode both on first launch.
- Raw Input no longer tracks non-gamepad HID collections on supported
  hardware IDs. The PlayStation Link adapter (054C:0ECC) exposes four
  vendor-defined collections that were logged as four tracked "DualSense"
  devices which could never send input, masking real detection problems.

## [0.6.2] - 2026-07-18

### Fixed

- Overlay preview no longer reads a stale gallery record after a fresh
  capture. `OverlayPreview` resolved the displayed record with an imperative
  `galleryModel.get()` call that no model signal re-evaluated, so a
  just-saved capture — which is prepended at row 0 — left the binding
  pointing at the previous capture. Two visible symptoms, one cause: X
  refused to start playback on a clip recorded moments earlier (the stale
  record reported a screenshot, so `toggleVideoPlayback` bailed out), and a
  fresh screenshot painted a play badge over the stage (the stale record
  reported a video). The record binding now tracks the same
  `_modelRevision` counter the target-URL binding already used, and that
  counter also advances on row removal and moves.

## [0.6.1] - 2026-07-18

### Added

- Overlay auto-hides on any OS foreground-focus change — pressing the Windows
  key, Alt-Tab, opening the task switcher, or clicking another app now closes
  the overlay automatically, the same way Circle or click-outside does.
  Implemented generically via a `SetWinEventHook(EVENT_SYSTEM_FOREGROUND)`
  watcher in `OverlayManager` rather than special-casing individual key
  combos, so it catches every system focus-changing operation. Unlike a
  normal close, this auto-hide does not force focus back onto the
  previously-focused game, since that would fight whatever the user just
  opened (Start menu, task switcher).

## [0.6.0] - 2026-07-18

### Added

- Save a still frame from a recorded clip. While a clip is focused for playback
  in the overlay or the desktop lightbox, pressing **Share** (or **S** on the
  keyboard) now grabs the exact frame currently on screen — paused on a chosen
  frame or mid-playback — and saves it as a screenshot, instead of capturing the
  whole desktop. The frame is taken at the clip's native resolution from the
  video surface, attributed to the clip's game, and written through the normal
  screenshot pipeline (same folder, format/quality, shutter sound, toast and
  gallery row). New `playback.frame_grab` action (Playback scope), so Share only
  changes meaning while a clip is focused and stays the global screenshot
  everywhere else.

## [0.5.99] - 2026-07-17

### Changed

- `DesktopGalleryGrid` no longer reaches into the main window. It now takes
  what it displays as properties (`columns`, `zoomLevel`, `bulkMode`,
  `bulkIsChecked`) and emits signals for what it wants done
  (`keyboardActivity`, `bulkToggleRequested`, `bulkDeleteRequested`,
  `bulkSelectAllRequested`), matching the pattern its sibling
  `DesktopGalleryHeader` already used. Gallery behaviour, including the pad
  bulk-selection flow and nav-lock timing, is unchanged.

## [0.5.98] - 2026-07-17

### Changed

- The "left stick doubles as the D-pad" rule now lives in one place
  (`input/StickNav.h`) instead of being hand-rolled in the DualSense, XInput
  and WinMM backends. Each backend keeps its own tuned deadzone values and its
  existing hysteresis behaviour, so pad navigation feels exactly as before;
  only the shared structure — axis polarity, mutually exclusive directions and
  the optional hysteresis — moved.

## [0.5.97] - 2026-07-17

### Added

- Settings → General → Appearance gained an "Overlay dimming" slider
  (25–150 %, default 100 %) that scales how strongly the in-game overlay
  darkens the game behind it. 100 % keeps each theme's own dimming; lower
  values keep more of the game visible, higher values darken it further
  (capped just short of opaque). Stored as `theme.overlay_scrim_strength`
  and applied live via the new `Theme.overlayScrim` token.
- New reusable `SettingsSlider` control: an integer-valued slider bound to a
  config key that follows the drag live but writes config only on release.

## [0.5.96] - 2026-07-17

### Fixed

- The desktop Lightbox no longer blanks the stage when a clip is selected. It
  now decodes the clip's thumbnail onto the still layer — as the overlay preview
  already did — and the video surface above it covers that once the player has a
  frame. Stepping quickly between captures in a mixed gallery no longer flickers
  the item away before the next one appears.

### Changed

- Internal: with both surfaces now keeping a decodable still behind a clip, the
  `MediaStage.qml` `clearOnEmptyTarget` flag that existed purely to tell them
  apart was removed; an empty target now always clears the committed still.

## [0.5.95] - 2026-07-17

### Changed

- Internal: the desktop Lightbox and the overlay preview now share a single
  `components/MediaStage.qml` for their double-buffered still/clip stage, with
  no intended behavior change. The shared component owns the parts that were
  genuinely identical — the async decode-then-promote handoff that keeps the
  previous capture painted while the next one decodes, and the media
  player/end-of-media wiring. The two surfaces disagree about what sits behind
  a clip (the Lightbox paints nothing there, the overlay keeps the clip's
  thumbnail up until playback is focused), so those rules stay explicit at each
  call site instead of being duplicated.

## [0.5.94] - 2026-07-17

### Changed

- Internal: `DualSenseDevice::parseReport` was split into named stages with no
  intended behavior change — report-layout offset lookup, button decoding,
  stick-to-D-pad decoding (hysteresis untouched), and active-pad routing.
  The stick deadzone values and the USB/BT/DS4 offsets are byte-for-byte the
  same; pending a pad-in-hand re-verification pass. Deadzone unification
  across XInput/WinMM stays deferred.

## [0.5.93] - 2026-07-17

### Changed

- Internal: the four large replay-pipeline functions were split into named
  phase helpers with no intended behavior change — `startPump` (device/item/
  recorder/session bring-up), `saveReplayOnWorker` (guard/freeze/thumbnail/
  export stages), `remuxConcatImpl` (segment open, writer setup, and one
  shared sample-copy loop for video and audio), and `SegmentRecorder::
  buildWriter` (sink creation plus video and audio stream setup). Pending a
  live-game re-verification pass of the replay buffer and clip saving.

## [0.5.92] - 2026-07-17

### Fixed

- The replay buffer no longer arms on fullscreen windows that are not games.
  A game was detected as "covers the monitor and is not a known shell
  process", which also matches every overlay — the Snipping Tool's screen-clip
  layer armed the buffer and recorded the desktop. Overlay windows are now
  rejected by their extended styles (layered, click-through, tool window, or
  no-activate), none of which a game's render window can carry, and the
  Snipping Tool's processes joined the shell blocklist.

## [0.5.91] - 2026-07-17

### Fixed

- The highlighted row in a Settings dropdown no longer paints its square
  corners over the popup's rounded ones. The list's `clip` only ever clipped
  to a plain bounding box, so the top and bottom rows overflowed the corner
  radius; the list is now masked to the popup's rounded shape.

## [0.5.90] - 2026-07-17

### Fixed

- A Share-hold (or `Ctrl+Shift+E`) made while the replay buffer was off now
  arms the buffer instead of only reporting "Replay buffer is not running".
  The gesture itself always fired on time; with nothing recording there was
  no footage to save, so the hold looked ignored and invited ever longer
  presses. The first hold now starts recording and says so, the next one
  saves a real clip.

## [0.5.89] - 2026-07-17

### Removed

- The `Ctrl+Shift+R` replay-buffer toggle hotkey. Always-on recording is
  enabled by default and its only control is now the switch in
  Settings → Replay — a stray key press can no longer silently persist
  recording off, which made replay saves fail with "Replay buffer is not
  running" while screenshots kept working.

## [0.5.88] - 2026-07-17

### Fixed

- The GameHQ logo at the bottom of the overlay sidebar now lines up with the
  icons of the entries above it, instead of sitting slightly further left.

## [0.5.87] - 2026-07-17

### Added

- The tray menu now has monochrome icons — a grid for Open Gallery, a circular
  arrow for Rescan, a camera for Take Screenshot, a clip for Save Replay, and a
  power mark for Exit. They are drawn from the menu's own palette rather than
  bundled as images, so they follow the Windows light/dark setting, and the
  labels no longer float beside an empty icon column.

## [0.5.86] - 2026-07-16

### Added

- **Options** opens Settings from anywhere in the gallery.
- **L2 / R2** shrink and grow the gallery thumbnails, repeating while held.
  The triggers are new bindable controls (L2/R2 on PlayStation, LT/RT on Xbox,
  ZL/ZR on Nintendo) — they were not addressable before.
- **Hold Cross** for a second enters bulk selection; **Square → Bulk select**
  does the same from the action menu.

### Changed

- Cross now confirms on tap-release rather than on press, so that holding it
  can mean something else. A quick tap behaves as before.

## [0.5.85] - 2026-07-16

### Fixed

- A focused Settings dropdown showed no highlight, so pad users could not tell
  where focus was and the control read as unreachable. Its border only lit on
  press or while open — never on focus, unlike every other settings control.

### Changed

- The PS5 theme is now called **Obsidian** and is the default theme.

## [0.5.84] - 2026-07-16

### Changed

- Settings now reads as three panels on a controller — sidebar │ categories │
  options. Left/Right moves between the panels; Up/Down moves inside the
  focused one and can no longer wander out of it. Cross flips a toggle or opens
  a dropdown; inside a dropdown Up/Down moves the highlight, Cross commits, and
  Circle backs out without changing the setting. Circle unwinds one step at a
  time: dropdown → options → categories → sidebar → out of Settings.
  Previously Left/Right switched category and Up/Down walked the raw focus
  chain straight across panel boundaries.

## [0.5.83] - 2026-07-16

### Changed

- The PS5 theme was too blue: its surfaces read as navy where the console's are
  near-black and almost neutral, and its backdrop bloom was a saturated
  blue/violet wash rather than the faint cool glow the real dashboard uses.
  Surfaces are now near-black with a slight cool cast, the bloom drops from 0.5
  to 0.2, and the violet orb is replaced by a cool teal.

## [0.5.82] - 2026-07-16

### Added

- New **PS5** theme, modeled on the PlayStation 5 dashboard: a near-black cool
  navy lit by a soft blue-violet bloom, pure white type, and generous spacing.
  The accents are Sony's published brand colors — PlayStation Blue (#003791) and
  X Blue (#0070D1) for the accent ramp, Triangle Green for success, Circle Pink
  for danger (circle is cancel/back on PlayStation), and Square Purple tinting
  the backdrop bloom. Type is Segoe UI, the closest match on Windows to Sony's
  proprietary SST.

## [0.5.81] - 2026-07-16

### Fixed

- After using a Settings dropdown (for example the theme picker), the Up/Down
  arrow keys kept driving that dropdown even after clicking elsewhere — so the
  arrows went on changing the theme from anywhere on the page. Clicking any
  non-interactive part of a settings page now drops keyboard focus, and the
  arrows only drive a control while that control is focused.

## [0.5.80] - 2026-07-16

### Fixed

- The app still died on startup (blank white window, then an access-violation
  crash) on every theme, not just the textured ones. 0.5.79 stopped the texture
  tile from exporting itself in a loop, but the export still ran from inside the
  canvas paint handler, which released the render target mid-paint. The export
  now runs once the canvas is idle.
- Themes with no texture (Dark, Light, High contrast, Cobalt, Emerald) no longer
  export an empty texture tile on every start.

## [0.5.79] - 2026-07-16

### Fixed

- The app crashed on startup (blank white window, then gone) when the active
  theme uses a background texture (Dracula, Gruvbox, Nord, Midnight, Harbor,
  Synthwave, Carbon). Exporting the generated texture tile re-triggered its own
  paint handler in an endless loop; it now exports exactly once per repaint.

## [0.5.78] - 2026-07-16

### Added

- Themes can now lay a faint texture over the background: film grain
  (Dracula, Gruvbox, Nord, Midnight), diagonal hatching (Harbor), a blueprint
  grid under Synthwave's scanlines, and a carbon-fiber weave for Carbon. The
  patterns are generated by the app itself -- no images shipped -- so they stay
  crisp at any window size and take their color from the theme. They are kept
  deliberately subtle: atmosphere behind your captures, never a pattern
  competing with them. Dark, Light, High contrast, Emerald and Cobalt are
  untouched.

## [0.5.77] - 2026-07-16

### Added

- Nine more themes, and they now change more than color. A theme can also set
  the typeface, how round the corners are, how heavy the borders are, how fast
  the app responds, and what gets painted behind everything -- so they actually
  feel different rather than just looking recolored.
  - **Midnight** -- near-black with soft blue light pooling behind the content;
    slow and cinematic.
  - **Emerald** -- charcoal and green, quick and immediate.
  - **Harbor** -- flat blue-grey with squared-off edges; deliberately plain.
  - **Carbon** -- layered greys and one bright blue; the quietest of the set.
  - **Cobalt** -- soft greys and indigo, very round, the fastest to respond.
  - **Synthwave** -- neon magenta and cyan over a violet horizon, with
    scanlines and a glow on anything selected.
  - **Nord**, **Dracula**, **Gruvbox** -- three well-known palettes; Gruvbox is
    warm and monospaced where the others are cool.
- **Dark is still the default and is unchanged.** Text sizes and spacing stay
  identical in every theme -- a theme restyles the app, it does not rearrange
  it, so nothing shifts under you when you switch.

## [0.5.76] - 2026-07-16

### Added

- You can now pick a theme. Settings -> General -> Appearance offers **Dark**
  (unchanged, still the default), **Light**, and **High contrast**, and the app
  repaints as soon as you choose -- no restart. Your choice is remembered.
- Only colors change. Text sizes, spacing and the video player's timings stay
  exactly as they were, because those are layout, not palette -- a theme should
  recolor the app, not rearrange it. A few things stay dark in every theme on
  purpose: the dimmed backdrop behind an opened capture, and the play badge and
  buttons drawn on top of video thumbnails. Those sit over your captures rather
  than over the app, and they have to stay readable whatever the frame shows.

## [0.5.75] - 2026-07-16

### Changed

- Developer documentation now matches the last two changes, and records a
  conclusion rather than a plan: the long-standing idea of merging the two video
  players into one shared piece was examined and dropped. They looked like
  duplicates but behave oppositely -- the full-screen viewer hides its still
  image behind the video, the overlay keeps showing the clip's thumbnail until
  you focus it -- so merging them would have meant one component full of
  switches standing in for about thirty genuinely shared lines. Writing down why
  it is not worth doing saves the next person from rediscovering it.

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
