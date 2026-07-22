# Portable-to-installed import inventory

This document defines the implemented path rewrite contract for importing a
portable GameHQ profile into a fresh installed profile. The entry point is
Settings > Advanced > Portable profile; GameHQ restarts before replacing any
profile files so the database is closed throughout publication.

`PortableProfileImporter` stages configuration and SQLite state beside the
installed data root, validates and publishes the directory atomically, and
writes count-only `import-evidence.json`. `tst_portableprofileimporter` covers
success, malformed and unsupported inputs, source immutability, path escapes,
destination rejection, derived-cache clearing, injected rollback points and
durable-journal recovery after hard interruption on either side of publication.

## Safety contract

- Accept only a package root containing `portable.flag`, `GameHQ.exe`, and
  `gamehq-data`.
- Resolve every `portable:/` value against that validated source package root;
  never resolve it against the installed program directory.
- Automatic v1 import targets only a new or empty installed profile. If an
  installed database or non-default configuration already exists, stop and
  offer an explicit backup-and-replace workflow later; do not merge libraries.
- Stage configuration and database changes beside the destination, validate
  them, then publish them atomically. Never edit the portable source in place.
- Keep capture media and watched folders where they are. Copy only profile
  state that belongs in installed AppData; rebuild disposable caches.
- Record source and destination snapshots before publishing. On any failure,
  delete staging and restore the untouched destination snapshot.
- Unknown configuration keys are preserved verbatim unless an audited rule
  explicitly identifies them as path-bearing. Emit a warning for an unknown
  string value beginning with `portable:/` instead of guessing.

## Persisted path inventory

| Persisted field | Current producer and consumer | Installed target | Rewrite rule | Rollback and fixture |
| --- | --- | --- | --- | --- |
| `config.json`: `storage.screenshots_root` | `CaptureLocations::setBaseRoot()` stores with `Paths::toStoredPath()`; `configuredRoot()` resolves it. | Existing source media root, or empty to use the installed default for new media. | Empty stays empty. `portable:/...` becomes an absolute path under the validated old package root. External absolute paths stay unchanged. | Restore destination config snapshot. Test empty, package-relative, external, missing, and traversal-rejected roots. |
| `config.json`: `storage.clips_root` | Same path contract as screenshots. | Existing source media root, or empty to use the installed default for new media. | Same rule as `storage.screenshots_root`. | Same fixtures as screenshots, with separate screenshot and clip roots. |
| `config.json`: `internal.capture_root_history[]` | `rememberPreviousRoot()` writes cleaned strings; `managedRoots()` resolves each entry. Historical entries may already be absolute. | Absolute paths to every retained historical media root. | Rebase each `portable:/...` entry to the old package root; preserve external absolute entries; deduplicate case-insensitively; append the old portable `Captures` root. | Restore destination config snapshot. Test mixed absolute/portable entries, duplicates, missing roots, and two custom roots. |
| `captures.file_path` | `insertCapture()` stores with `Paths::toStoredPath()`; `CaptureQueries` reads with `repairMovedPath()`. | Original capture file beneath the old portable package or its existing external location. | Rebase `portable:/...` to an absolute source path. Do not move media to `Videos\\GameHQ`. Preserve external absolute paths. Reject package escape after canonicalization. | Import a favorite screenshot and video, verify bytes/hash and metadata, then force rollback and prove source and destination snapshots are unchanged. |
| `captures.thumbnail_path` | Thumbnail generation writes beneath `gamehq-data/thumbnails`; database reads use `repairMovedPath()`. `CaptureScanner` recreates missing thumbnails. | Installed thumbnail cache. | Set imported values to `NULL`; do not copy or reference the portable cache. Regenerate on the next scan from the rewritten capture path. | Fixture includes present, missing, legacy clip, and corrupt thumbnails; rollback restores the destination database. |
| `games.executable_path` | Game discovery writes the real game executable path; it is intentionally not passed through `Paths` repair. | Same external game executable. | Preserve absolute path unchanged. A `portable:/` value is invalid for this field and must stop import with a diagnostic. | Test existing and missing game executables plus an invalid portable-prefixed value. |
| `games.icon_path` | `GameIconCache` writes beneath `gamehq-data/game-icons`; startup refresh derives icons from `executable_path`. | Installed game-icon cache. | Set imported values to `NULL` and remove `internal.icon_format` from the staged database so startup re-extracts icons. Do not copy the portable cache. | Test a normal Win32 icon, a packaged-game icon, a missing executable, and rollback before publish. |
| `folders.path` | `addWatchedFolder()` stores with `Paths::toStoredPath()`; scans read with `repairMovedPath()`. | Original watched folder. | Rebase `portable:/...` to an absolute old-package path; preserve external absolute paths; canonicalize and deduplicate. | Test package-local, external, duplicate, missing, and traversal-rejected folders. |
| `sound_settings.sound_file` | Schema is present, but current code has no reader or writer. | Copied installed profile asset only when the source exists inside `gamehq-data/sound-packs`; otherwise its existing external path. | For `portable:/gamehq-data/sound-packs/...`, copy the referenced file into installed `sound-packs` staging and rewrite to the installed absolute path. Rebase other package-relative files to the old root without copying. Preserve external paths. | Test copied pack, external file, missing file, package escape, and copy failure with full rollback. |
| `settings` table | Current persisted keys are non-path sentinels such as `internal.repairs_v1_done` and `internal.icon_format`. | Installed database. | Preserve all except delete `internal.icon_format` when clearing imported icon paths. Treat future string values beginning with `portable:/` as an unsupported schema requiring an importer update. | Fixture includes known sentinels and an unknown portable-prefixed value that must fail closed. |

## Filesystem-owned directories

| Source content | Import policy | Reason |
| --- | --- | --- |
| `gamehq-data/config.json` | Parse, rewrite audited fields, and stage a new file. | A raw copy leaves `portable:/` values unresolved in installed mode. |
| `gamehq-data/gamehq.db` | Copy to staging, run rewrites in one SQLite transaction, then validate schema and foreign keys. | Preserves library metadata without mutating the source database. |
| `Captures/` and custom capture roots | Do not copy or delete; retain as absolute scan roots. | Media can be large and must remain recoverable in its original location. |
| `gamehq-data/thumbnails/` | Do not copy. | Derived cache is recreated by `CaptureScanner` and `ThumbnailService`. |
| `gamehq-data/game-icons/` | Do not copy. | Derived cache is recreated from `games.executable_path`. |
| `gamehq-data/replay-cache/` | Do not copy. | Ephemeral rolling-buffer segments must never survive migration. |
| `gamehq-data/logs/` | Do not copy by default; optionally attach a copy to import evidence. | Logs are diagnostic evidence, not live installed state. |
| `gamehq-data/sound-packs/` | Copy only files referenced by `sound_settings.sound_file`. | Avoids silently importing unused or unknown executable content. |

## Transaction and validation order

1. Validate and canonicalize the source package root; reject symlink/junction or
   canonical paths that escape it.
2. Confirm the installed destination profile is new or empty, then snapshot it.
3. Copy `config.json` and `gamehq.db` to a unique staging directory.
4. Rewrite audited configuration fields and fail closed on unknown
   `portable:/` values.
5. Open the staged database, begin one transaction, rewrite the audited
   columns, clear disposable cache paths, and commit.
6. Run `PRAGMA integrity_check`, `PRAGMA foreign_key_check`, schema-version
   compatibility checks, and a dry resolution pass over every imported path.
7. Publish staged profile files atomically. Leave source media and profile
   untouched and write an evidence report containing counts, not private paths.
8. On any error before or during publish, restore the destination snapshot and
   remove staging; never attempt a partial merge.

## Required implementation fixtures

The importer test suite must include: a default portable profile; split custom
screenshot and clip roots; historical and watched roots; screenshots and clips
with favorites; missing media; stale thumbnails; game icons; external game
executables; a referenced sound pack; Unicode and long paths; mixed separators;
case-only duplicates; a newer unsupported schema; malformed JSON/SQLite; a
`portable:/../` escape attempt; destination-not-empty rejection; injected copy,
database, validation, and publish failures; and a successful retry after every
rollback case.
