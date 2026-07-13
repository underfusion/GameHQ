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

## Clean package layout

Both `build/` and `dist/GameHQ/` use this root layout:

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

`start.bat` automates the first two commands. It incrementally builds in `out/`,
stops the running app, refreshes only generated content in `build/`, preserves
`build/Captures/` and `build/gamehq-data/`, then launches the root executable.

`make-dist.ps1` wraps the same assembler but recreates `dist/GameHQ/` from
scratch. The assembler validates its destination, copies the real executable
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
