# Storage & Folders

## Managed capture trees

```txt
<ScreenshotsRoot>/<Game Name>/Screenshots/*.png
<ClipsRoot>/<Game Name>/Clips/*.mp4
<EitherRoot>/Unknown Game/{Screenshots,Clips}/
```

Screenshots and clips share the same default root: `%USERPROFILE%\Videos\GameHQ` (never Game Bar's `Videos\Captures`). Full-portable mode defaults both to `Captures/` next to the portable root. Settings may override them independently through `storage.screenshots_root` and `storage.clips_root`; an empty value means the current portable/installed default, so moving a portable package does not bake in its old absolute path.

Changing a root affects new writes only. GameHQ never auto-moves or deletes existing media. The default root and every previous custom managed root remain in an internal scan history, including after a per-location reset or Restore all settings, so rebuilding/rescanning the library can still find earlier captures. Duplicate or shared roots are de-duplicated before scanning.

## Portable resolution (`Paths.cpp`)

1. `portable.flag` next to the exe (dev build tree) ⇒ everything under `<exe>/gamehq-data/` + `<exe>/Captures/`.
2. `portable.flag` in the exe's **parent** directory (dist package: root launcher + `app/GameHQ.exe`, see [packaging.md](packaging.md)) ⇒ data at the package root: `<root>/gamehq-data/` + `<root>/Captures/`.
3. Otherwise: data in `%APPDATA%\GameHQ\`, default screenshot and clip output in `Videos\GameHQ`.

`gamehq-data/`: `config.json`, `gamehq.db`, `thumbnails/`, `game-icons/` (cached foreground executable icons, including icons recovered from historical detector logs), `logs/`, `replay-cache/`, `sound-packs/`.

## Legacy name adoption (`LegacyMigration.cpp`)

The app was previously called SavePlay, then PlayHQ ([branding.md](branding.md)). Old installs are adopted on first start, and all of it lives in `LegacyMigration`:

- **Data folder** — `saveplay-data`/`playhq-data` (portable) or `%APPDATA%\SavePlay\SavePlay`/`%APPDATA%\PlayHQ\PlayHQ` (installed) is renamed to the current path, but only when the current one does not exist. If the rename fails (e.g. the folder is locked), the legacy folder is used in place rather than starting empty.
- **Database** — a `saveplay.db`/`playhq.db` next to it is renamed to `gamehq.db` under the same condition; `Paths::databasePath()` is the only way in.
- **Capture root** — resolved read-only: an existing `Videos\SavePlay`/`Videos\PlayHQ` keeps being used as-is. User media is never moved.

Each helper is a no-op once the current path exists, so later starts cost only an existence check. Registry Run-key entries under the old names are cleaned up separately by `StartupManager`.

## Watched/import folders

Game Bar Captures, Steam Screenshots, NVIDIA Gallery, OBS, custom — rows in `folders` table, captures tagged with `source`. **Imported folders are read-only unless the user explicitly enables management.**

## Cleanup rules (0.7)

Max storage 10/25/50/100 GB/unlimited · auto-cleanup modes: off / oldest non-favorites / clips only / both · **favorites never auto-deleted** · confirm before deleting anything outside managed folders · warn on low disk.
