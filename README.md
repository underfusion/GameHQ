<div align="center">

<img src="assets/icons/gamehq.svg" width="96" alt="GameHQ icon">

# GameHQ

**Save the moments that already happened — screenshots, recent-gameplay video clips, and an in-game capture gallery for Windows.**

[![Release](https://img.shields.io/github/v/release/underfusion/GameHQ?display_name=tag&style=flat-square)](https://github.com/underfusion/GameHQ/releases/latest)
[![Windows](https://img.shields.io/badge/Windows-10%201903%2B-3578e5?style=flat-square)](#requirements)
[![License](https://img.shields.io/github/license/underfusion/GameHQ?style=flat-square)](LICENSE)

</div>

![GameHQ gallery](docs/assets/gamehq-gallery.png)

GameHQ brings console-style capture controls to PC gaming. While you play, it
continuously keeps the most recent gameplay in a rolling buffer. When something
worth saving happens, press one button **after the moment** to turn the previous
configurable minutes into a normal MP4 video clip — there is no need to start
recording beforehand. You can also take instant screenshots and browse everything
in a controller-friendly gallery without leaving your game.

## Highlights

- **Instant screenshots** in PNG or JPEG, organized automatically by game.
- **Record recent gameplay after it happens**: GameHQ continuously buffers the
  previous configurable minutes, then saves them as an MP4 video with system
  audio when you press the replay button. Duration, quality, frame rate, and
  resolution are configurable.
- **In-game overlay** for browsing, playing, favoriting, revealing, and deleting
  captures without alt-tabbing. It refreshes immediately after a new capture and
  closes cleanly when Windows focus moves to another app, Start, or the task switcher.
- **Save a frame from any clip**: while a recorded clip is focused, press Share
  or `S` to turn the exact displayed frame into a normal screenshot assigned to
  the same game.
- **Controller-first controls** with DualSense, XInput, WinMM, keyboard, and safe
  extra mouse-button support. Settings can detect Sony controllers hidden by
  HidHide and safely add GameHQ to its application allow-list.
- **Fully configurable bindings** with primary/secondary slots, tap, hold,
  double-tap, conflict handling, per-controller profiles, and restore controls.
- **Thirteen themes**, with Obsidian as the fresh-install default, live theme
  switching, textured backdrops, and adjustable in-game overlay dimming.
- **Controller-friendly library tools** including thumbnail zoom on L2/R2 and
  bulk selection by holding Cross or opening the capture action menu.
- **Unified library** for GameHQ captures plus read-only watched folders from
  tools such as Steam, Xbox Game Bar, NVIDIA, and OBS.
- **Portable and private**: no account, telemetry, game-process injection, or
  background service. Settings and captures can stay beside the app.

## Download

Download only from the official
[GameHQ Releases page](https://github.com/underfusion/GameHQ/releases). Choose
`GameHQ-<version>-win64-setup.exe` for the recommended per-user installation, or
the `-portable.zip` package to keep data beside the app.

**Open-source Beta · Not yet code-signed; Windows may show an Unknown publisher
warning.** Verify the official source and published release hash before running
an artifact. A specific Defender malware/PUA detection is different: do not
bypass it or disable Windows security. See
[Download verification](docs/download-verification.md) and
[Security & privacy](docs/security-and-privacy.md).

## Default controls

| Input | Action |
|---|---|
| Share / Capture — tap | Take a screenshot |
| Share / Capture — hold | Save the replay buffer as a clip |
| PS / Guide | Open or close the in-game overlay |
| D-pad / left stick | Navigate |
| Cross / south button | Confirm or open |
| Circle / east button | Back |
| L2 / R2 | Decrease or increase gallery thumbnail size |
| Cross / south button — hold | Enter bulk selection in the desktop gallery |
| Share / `S` while playing a clip | Save the displayed video frame as a screenshot |
| `Ctrl+Shift+S` | Take a screenshot |
| `Ctrl+Shift+E` | Save a replay clip |
| `Ctrl+Shift+G` | Toggle the overlay |

Bindings and gesture timing can be changed from **Settings → Input**.

## Requirements

- Windows 10 version 1903 or newer, 64-bit.
- A GPU and driver supporting Windows Graphics Capture and H.264 encoding.
- Enough free disk space for the configured rolling buffer and saved captures.

## Building from source

GameHQ uses C++20, Qt 6.8.3/QML, CMake, Ninja, SQLite, Windows Graphics Capture,
Media Foundation, WASAPI, and Windows input APIs.

See [Development Setup](docs/dev-setup.md) for the toolchain and
[Packaging & Distribution](docs/packaging.md) for the portable package layout.

```powershell
tools/cmake/bin/cmake.exe -S . -B out -G Ninja
tools/cmake/bin/cmake.exe --build out
powershell -ExecutionPolicy Bypass -File packaging/make-dist.ps1
```

## Documentation

Architecture and subsystem documentation lives in [docs/](docs/README.md).
Changes are summarized in the [changelog](CHANGELOG.md).
Vulnerabilities can be reported privately through the repository's enabled
[security advisory form](https://github.com/underfusion/GameHQ/security/advisories/new).

## License

GameHQ source code is available under the [MIT License](LICENSE). Distributed
packages also contain Qt, FFmpeg, and compiler runtime components under their
respective licenses; see [Third-Party Notices](THIRD_PARTY_NOTICES.md).

GameHQ is an independent project and is not affiliated with or endorsed by
Sony Interactive Entertainment, Microsoft, Valve, NVIDIA, or OBS Project.
