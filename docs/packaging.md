# Packaging & Distribution

How GameHQ separates compiler output from the clean portable folders used for
local runs and downloads, and how the application icon is produced.

## Build layers

GameHQ uses three separate trees so generated development files never appear in
the package a user opens:

| Path | Purpose | User-facing |
|---|---|---|
| `out/` | CMake/Ninja cache, generated sources, object files, and freshly linked executables | No |
| `build/` | Clean local portable package launched by `start.bat`; runtime data is preserved when refreshed | Yes, local |
| `dist/GameHQ/` | Clean release package rebuilt from scratch for sharing or archiving | Yes, release |

This follows Qt's Windows deployment guidance: `windeployqt` creates a runtime
tree containing the required libraries, QML imports, plugins, and translations.
Qt resolves plugins relative to the real executable, so the runtime belongs
together under `app/`; it must not be scattered through the package root.

Reference: [Qt for Windows - Deployment](https://doc.qt.io/qt-6/windows-deployment.html).

The static update helper uses miniz 3.1.2, fetched from its tagged upstream
archive with a pinned SHA-256 during configuration. It accepts only stored or
deflated ZIP entries and applies GameHQ's own path, size, layout and manifest
validation before staged files can be considered installable. Its MIT license
is included at `licenses/miniz.txt`.

`packaging/prepare-source.ps1` can generate a revision-bound source ZIP and
checksum as an optional release convenience. `packaging/validate-source.ps1`
checks its layout, hash, and clean build. MIT binary packages do not require an
embedded source offer.

## Distribution identity contract

[`packaging/distribution-identity.psd1`](../packaging/distribution-identity.psd1)
is the machine-readable source for packaging and installer identity. Its
`InnoAppId` is permanent: `{1CB27009-5809-408F-8510-1C4F19605565}`. GameHQ is
a per-user product with product ID `underfusion.gamehq`, publisher
`underfusion`, and default installed root
`%LocalAppData%\Programs\GameHQ`. Installed packages omit `portable.flag`.

Setup owns these current-user values:

```text
HKCU\Software\underfusion\GameHQ
  InstallLocation = <absolute installation root>
  Version = <GameHQ VERSION>

HKCU\Software\Microsoft\Windows\CurrentVersion\App Paths\GameHQ.exe
  (Default) = <absolute installation root>\GameHQ.exe
  Path = <absolute installation root>
```

The App Paths entry is part of the installer contract. Autostart remains a
user setting at `HKCU\Software\Microsoft\Windows\CurrentVersion\Run` value
`GameHQ`; installing GameHQ does not enable it. Setup may update only the
values above. Uninstall removes a value only when it still points to the same
installation, and removes the `GameHQ` autostart value only when it is owned by
that installation. It never removes unrelated values or user data.

Discovery never scans disks. A consumer checks an established
`GameHQ.Local.v1` connection, a manually selected launcher, `InstallLocation`,
App Paths, the current-user Run value, then the default installed root. A valid
candidate contains root `GameHQ.exe` and `app\GameHQ.exe`. The global
`%TEMP%\gamehq.lock` continues to prevent simultaneous portable and installed
copies, while the updater helper always runs from the application root.

Version-one uninstall removes installed program files, shortcuts, its uninstall
entry, and only integration values that still point to that exact installation.
It removes an owned `GameHQ` autostart value to avoid a dead launch entry, but
preserves changed or unrelated registry values. It never presents a data-purge
option and never removes AppData, `Videos\GameHQ`, historical capture roots,
portable folders, or media. A future data purge must be a separate in-app tool
with its own inventory and explicit confirmation.

Release artifacts are named from the same data file:

```text
GameHQ-<version>-win64-setup.exe
GameHQ-<version>-win64-portable.zip
GameHQ-<version>-win64-update.zip
GameHQ-<version>-win64-update.zip.sha256
[optional] GameHQ-<version>-source.zip
[optional] GameHQ-<version>-source.zip.sha256
gamehq-release.json
gamehq-release.sig
```

The installer compiler is pinned separately in
`packaging/inno-toolchain.psd1`. `bootstrap-inno.ps1` downloads the immutable
official Inno Setup 6.7.3 release, verifies both its SHA-256 and expected
Authenticode signer, and installs it in portable mode below `tools/`.
`build-setup.ps1` compiles the full offline Setup exclusively from the neutral
payload. No web bootstrapper, service, scheduled task, Defender exclusion, or
automatic HidHide elevation is part of Setup.

Setup targets x64-compatible Windows 10 1903 or newer and installs per-user
without elevation. It publishes the complete version, publisher, support,
updates and uninstall-icon metadata. It never force-closes GameHQ or restarts
it through Restart Manager. Silent automation treats all nonzero Inno codes as
failure. GameHQ reserves code `20` for a running application and `21` for an
active update/recovery transaction; trust and payload gates are added by their
owning release task. The accepted GameHQ process holds
`Local\GameHQApplicationActive` for its whole lifetime. Setup and Uninstall use
the matching Inno `AppMutex`, never ask Restart Manager to terminate the
process, and require a normal user-initiated shutdown before replacing files.

## Neutral payload and package layout

`assemble-package.ps1 -Mode Neutral` creates `dist/.program-payload/` as the
single flag-free program tree. Portable staging copies it and adds
`portable.flag`; installer staging consumes it unchanged; update staging copies
only its allowlisted program files and renames the updater to its pending name.
The neutral payload is validated to contain no capture or configuration data.

`build/` and `dist/GameHQ/` use this root layout. `portable.flag` appears only
in a portable package; an installed layout omits it, and update-only packages
must never contain or modify it:

```txt
GameHQ/
  GameHQ.exe      tiny static Win32 launcher; starts app\GameHQ.exe
  README.txt      user-facing readme with the current version
  portable.flag   keeps settings, captures, and logs inside this folder
  app/            real exe + Qt/FFmpeg/MinGW DLLs + plugins + QML runtime
  Captures/       created at runtime - screenshots and clips by game
  gamehq-data/    created at runtime - config, DB, thumbnails, and logs
```

Windows requires load-time DLLs next to the executable that loads them. The
launcher keeps those files one level down while leaving one obvious executable
at the root. `Paths.cpp` accepts `portable.flag` beside the real exe or one level
above it, so both package folders store user data at their clean root.

## Configure, build, and package

Configure once into the developer-only `out/` tree (full toolchain command is in
[dev-setup.md](dev-setup.md)), then build and assemble either package:

```txt
tools/cmake/bin/cmake.exe --build out
powershell -ExecutionPolicy Bypass -File packaging/assemble-package.ps1 -BuildDirectory out -Destination build -PreserveUserData
powershell -ExecutionPolicy Bypass -File packaging/make-dist.ps1
```

A release build must select its trust state explicitly, for example:

```powershell
$env:RELEASE_TRUST_MODE = 'unsigned-beta'
powershell -ExecutionPolicy Bypass -File packaging/make-dist.ps1
```

The command builds Setup, Portable and Update together and writes
`release-evidence.json` with final hashes, sizes, trust state, Authenticode
observations and the pinned Inno version/checksum. `unsigned-beta` requires
honest Beta wording and consistently unsigned GameHQ-built binaries; `signed`
rejects a missing/invalid signature or inconsistent publisher.
`-ManifestMode test` exercises the reviewed Ed25519 generator and all three
verification consumers with an explicitly public test key. Production key
activation and public signed-manifest mode remain separately gated.

The `Unsigned Beta release gate` workflow repeats that exact path on a clean
Windows runner for pull requests and pushes to `dev` or `main`. Before upload,
`assert-beta-artifact-set.ps1` requires the exact seven-file CI artifact set and
recomputes every size and SHA-256 value recorded in `release-evidence.json`.
The uploaded bundle is short-lived CI evidence, not a GitHub Release. A
separate negative step proves that a publishable tag cannot validate while the
public test key is selected. Production signing configuration remains absent
until the separately authorized production-key and Authenticode tasks.

Installed builds expose **Settings > Advanced > Portable profile**. Import is
available only when the installed library is empty. GameHQ closes the live
database, restarts into a dedicated importer, rebases audited `portable:/`
paths, clears derived thumbnail/icon references, validates SQLite, then swaps
the staged profile into place. Capture media and the portable source remain
untouched; any failure restores the original empty installed profile.

The importer uses a persistent transaction journal so restart recovery can
finish rollback after interruption. It never merges two populated libraries,
moves source media, rewrites unknown `portable:/` fields, or deletes the source.
Derived thumbnails and game icons are rebuilt in installed storage. The UI
shows the source and destination before confirmation and reports the evidence
or recovery location when the operation cannot complete.

Uninstall changes neither data ownership nor license rights. AppData, capture
media, watched folders, and portable profiles remain user-controlled, and the
MIT grant continues to apply to every downloaded GameHQ version distributed
under it.

`start.bat` automates the first two commands. It incrementally builds in `out/`,
stops the running app, refreshes only generated content in `build/`, preserves
`build/Captures/` and `build/gamehq-data/`, then launches the root executable.

`make-dist.ps1` first recreates the neutral payload, derives `dist/GameHQ/`,
then creates `dist/releases/GameHQ-<version>-win64-portable.zip` plus the
program-only `GameHQ-<version>-win64-update.zip` and checksum, then runs
`validate-release.ps1`. The gate checks filenames, manifest/version agreement,
checksum, neutral/ZIP allowlists, packaged binary version and updater tests.
The assembler validates its destination, copies the real executable
into `app/`, runs `windeployqt --qmldir src/ui/qml --compiler-runtime`, adds the
FFmpeg DLLs that Qt Multimedia loads dynamically, then writes the launcher,
readme, and portable marker at the root.

## App icon

- Source of truth: `assets/icons/gamehq.svg`.
- `assets/icons/gamehq.ico` carries true per-size renders (16-256 px); its
  regeneration recipe is in `assets/icons/icongen.cpp`.
- `assets/icons/gamehq.rc` embeds the ICO into both executables for Explorer,
  taskbar, tray, and shortcut use.
- At runtime `main.cpp` and `TrayIcon.cpp` use `:/icons/gamehq.ico`; QML sidebars
  keep the SVG for crisp scaling.
- The top-level CMake file locates MinGW's `windres.exe` beside the configured
  C++ compiler before enabling resource compilation.
