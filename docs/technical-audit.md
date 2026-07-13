# Technical Audit

Date: 2026-07-09. Scope: current desktop app, overlay, capture/replay, storage, QML, and project documentation.

## Current Shape

GameHQ is a Qt/QML Windows app with a native C++ service layer:

- `App` wires lifetime and signals for config, database, scanner, galleries, screenshots, replay frame pump, overlay, notifications, input, sounds, tray, and QML context properties.
- `AppController` is the single QML bridge. It owns QML-facing filter state, folder commands, and config proxy methods while delegating capture-library work, current-game lookup, and shell actions to focused helpers.
- `GalleryModel` is a thin `QAbstractListModel` over `CaptureDatabase::listCaptures(category, gameId)`.
- `CaptureDatabase` owns the public storage facade, SQLite schema/migrations, mutations, input bindings, watched folders, and insert-time game resolution. Read queries and startup repair/icon work live in storage helpers.
- `GameDetector` resolves the foreground window, process path, likely game title, fullscreen heuristic, and capture gate.
- `ScreenshotService` handles one-shot GDI screenshots and async PNG writes.
- `FramePumpService` handles always-on WGC replay capture on a dedicated MTA worker thread, rolling H.264/AAC segments, and replay export.
- `OverlayManager` owns lazy overlay QML loading, topmost window placement, foreground stealing, and focus restoration.
- `InputEngine` routes controller input between global actions, overlay, and desktop gallery.

The code is functional and still small enough to evolve without a full rewrite. The highest-risk area is not algorithmic complexity; it is responsibility drift across `AppController`, `CaptureDatabase`, and duplicated QML navigation/category logic.

## Findings

### P0 - No Immediate Blocker Found

The current dirty tree builds. I did not find a compile-time blocker during this audit.

### P1 - `AppController` Is Becoming a God Bridge

`AppController` is the only intended QML bridge, which is good, but it now mixes too many responsibilities:

- main gallery filtering
- overlay-gallery filtering helpers
- foreground/current-game state
- capture insertion and thumbnail commit
- bulk/single file deletion
- shell actions
- watched-folder commands
- config proxy methods

Risk: every new UI feature is likely to add another method/property here, making behavior harder to test and increasing the chance that desktop and overlay state drift apart.

Recommended split:

- Keep `AppController` as a facade for QML.
- Move capture file operations into `CaptureActions` or `CaptureLibraryService`.
- Move current-game resolution into `CurrentGameService`.
- Move open/show-in-folder into a tiny `ShellActions` helper.
- Let the facade expose stable QML properties while delegating work internally.

### P1 - Game Name Normalization Is Duplicated

Folder-safe game key logic exists in multiple places:

- `CaptureDatabase.cpp::folderSafeGameKey`
- `AppController.cpp::folderSafeGameKey`
- `ScreenshotService.cpp::sanitizeFolder`
- `FramePumpService.cpp::sanitizeFolder`

Risk: one future change to title normalization or invalid-character handling can produce duplicate game rows or mismatched capture folders.

Recommended split:

- Add `src/games/GameIdentity.{h,cpp}` or `src/storage/CapturePath.{h,cpp}`.
- Centralize display name fallback, folder-safe name, case-folded identity key, and captures subfolder paths.

### P1 - Category Lists Are Duplicated in QML

Desktop `Main.qml` and `OverlayWindow.qml` both build category arrays and both special-case the dynamic `Game` row.

Risk: future category changes will be implemented twice and can diverge. The recent `Game` section request already hit this class of problem.

Recommended split:

- Add a small QML component/helper, for example `SidebarCategories.qml`, or expose categories from C++ as a model.
- Keep the `Game` row rule in one place: visible only when `app.currentGameAvailable`, maps to `app.currentGameId`, otherwise `All` remains first.

### P1 - `CaptureDatabase` Has Too Many Non-Database Concerns

The DB class now handles schema, queries, migrations, game de-duplication, icon extraction, and log-based backfill.

Risk: migration/query changes become entangled with filesystem/icon/log behavior. Startup can also become slower as repair logic grows.

Recommended split:

- Keep SQL in `CaptureDatabase`.
- Move executable icon extraction to `GameIconCache`.
- Move historical log repair to `GameMetadataBackfill`.
- Add small query helpers such as `hasCapturesForGame(int gameId)` instead of using `listCaptures(...).isEmpty()` for existence checks.

### P2 - QML Files Are Large

Largest QML files at the start of the audit:

- `Main.qml`: about 1,391 lines
- `OverlayWindow.qml`: about 911 lines before the overlay component split
- `SettingsView.qml`: about 367 lines
- `Lightbox.qml`: about 353 lines
- `PlayerControls.qml`: about 329 lines

Risk: navigation, rendering, actions, and modal logic live together, making small UI changes risky.

Recommended split:

- Main desktop: `SidebarPanel.qml`, `GalleryToolbar.qml`, `CaptureGrid.qml`, `ActionMenu.qml`.
- Overlay: `OverlaySidebar.qml`, `OverlayPreview.qml`, `OverlayStrip.qml`, shared `ActionMenu.qml` where practical.

### P2 - Documentation Drift Exists

Stale or partially inaccurate docs found during audit:

- `docs/database.md` said DB access is serialized on a worker thread. Current implementation uses direct Qt SQL calls from the owning app objects.
- `docs/storage.md` said first launch lets the user choose captures root. Current `Paths.cpp` uses portable mode or default Videos/GameHQ without a chooser.
- `docs/capture-engine.md` still stated exclusive-fullscreen GDI screenshots are expected to be black, while the tracker says real game testing verified non-black captures on this machine.
- `docs/architecture.md` described several planned classes (`AppState`, `StorageManager`, `TopmostWindow`, `FocusManager`) as if they exist.

This audit updates the broad documentation direction, but future code changes should continue to update the feature-specific docs in the same change.

### P2 - Tests Are Mostly Manual

The app relies on build/run/log verification and real hardware/game tests. That is expected for WGC, controller, and overlay behavior, but some pure logic can be covered automatically.

Good first automated tests:

- game folder/name normalization
- `CaptureDatabase` insert/list/favorite/delete behavior using a temp DB
- category filtering
- config default overlay
- replay length/resolution parsing helpers

## Refactor Plan

### Phase 1 - Low-Risk Extraction

Goal: reduce duplication without changing behavior.

Status: completed in dev.95.

1. Added `core/GameIdentity` for folder-safe names and identity keys.
2. Replaced duplicated `folderSafeGameKey` / `sanitizeFolder` call sites.
3. Added `CaptureDatabase::hasCapturesForGame(int gameId)`.
4. Replaced current-game `listCaptures("all", gameId).isEmpty()` existence checks.
5. Added `ui/qml/helpers/SidebarCategories.js` for desktop and overlay sidebar categories.

Verification:

- Build.
- Launch.
- Confirm current-game `Game` section appears/disappears correctly.
- Confirm existing Khazan captures still map to one game row.

### Phase 2 - AppController Facade Cleanup

Goal: keep QML API stable while moving work behind the facade.

Status: done for the current safe wave. `ShellActions` owns platform shell calls, `CaptureLibraryService` owns capture delete/open/show-in-folder plus screenshot/clip commit bookkeeping, and `CurrentGameService` owns foreground/current-game matching behind the existing `AppController` invokables/properties.

1. Extract file deletion/open/show-in-folder into `CaptureLibraryService`. Done.
2. Extract capture commit paths into `CaptureLibraryService`. Done.
3. Extract foreground/current-game logic into `CurrentGameService`. Done.
4. Keep `AppController` properties and invokables stable unless QML is changed in the same commit.

Verification:

- Build and launch.
- Screenshot save.
- Replay save if a game is available.
- Bulk delete with confirmation.
- Overlay and desktop gallery refresh after capture.

### Phase 3 - Database Boundary Cleanup

Goal: make DB code mostly SQL and migrations.

Status: completed for the current safe wave. `GameIconCache` owns executable icon extraction, `GameMetadataBackfill` owns historical log repair, `GameRowRepair` owns startup duplicate game-row merging, and `CaptureQueries` owns read-only capture/game listing plus boolean lookup SQL behind the stable `CaptureDatabase` API.

1. Move game icon cache into `GameIconCache`. Done.
2. Move log-based metadata repair into `GameMetadataBackfill`. Done.
3. Move duplicate game-row repair into `GameRowRepair`. Done.
4. Move read-only capture/game query helpers into `CaptureQueries`. Done.
5. Keep migrations explicit. If schema changes, add v2 instead of editing v1.
6. Add small query methods instead of making callers load full record lists for boolean checks.

Verification:

- Existing database opens.
- New database initializes.
- Older v1 database repairs metadata columns.
- Game icons still cache and display.

### Phase 4 - QML Decomposition

Goal: make UI changes smaller and safer.

Status: completed for the current safe wave. Desktop sidebar/header/grid/footer/empty-state and overlay sidebar/footer/capture-strip/preview are now componentized; the action menu component is shared by desktop and overlay.

1. Extract desktop sidebar and toolbar first. Done.
2. Extract overlay sidebar second. Done.
3. Extract overlay capture strip. Done.
4. Extract shared action menu only if desktop/overlay behavior remains similar. Done.
5. Extract desktop empty state. Done.
6. Extract overlay preview/video playback when the player boundary is clear. Done.
7. Extract desktop gallery grid surface. Done.
8. Keep all visual constants in `Theme.qml`.

Verification:

- Desktop controller and mouse navigation.
- Overlay controller navigation.
- Bulk select.
- Lightbox/video controls.

## Suggested Order

The current safe refactor wave is complete. Do not continue with a broad rewrite. Future cleanup should be driven by concrete feature work or an isolated pain point, keeping public QML and C++ APIs stable.
