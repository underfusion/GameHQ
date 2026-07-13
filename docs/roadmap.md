# Roadmap

This document describes product direction. Shipped behavior is documented in
the changelog and current subsystem documentation.

## 0.1 — Native Gallery MVP
Prove the gallery beats Xbox Game Bar. Portable C++/Qt app, SQLite, folder scan, large preview, thumbnails, favorites, grouping, basic controller nav, tray, settings.
**Accept:** portable launch · user picks folder · images+videos in gallery · keyboard/controller browsing · favorites persist across restart · thumbnails cached · tray works.

## 0.2 — Overlay Shell MVP
Core in-game overlay before capture features. Separate frameless topmost window, hotkey + controller toggle, appears above borderless game, Circle/back closes, focus returns to game, nav + open/close sounds.
**Accept:** opens above borderless-fullscreen game · closes with PS/Circle · never opens desktop window · controller nav works · game ignores controller while overlay focused · focus restored · sounds play.

## 0.3 — Input System
DualSense detection, Share tap/hold, PS detection (best effort), global hotkeys, bindings model, binding test screen, tap-vs-hold, overlay routing.
**Accept:** tap and hold trigger distinct test actions · tap never fires after hold · hotkeys global · bindings persist · PS optional · overlay consumes input.

## 0.4 — Screenshot Capture
WGC screenshot, game detection, PNG to per-game folder, DB, thumbnail, sound, notification, only-in-games gate.
**Accept:** Share tap + hotkey both save · correct game folder · appears in gallery immediately · metadata in SQLite · sound plays · blocked outside games.

## 0.5 — Replay Buffer MVP
1080p30 H.264, 5-min buffer of 10-s segments, WASAPI audio, Share-hold export, MP4, gallery, sound, notification.
**Accept:** buffer runs while gaming · segments rotate & old ones deleted · hold exports last 5 min · clip plays with audio · in gallery · no stray screenshot.

## 0.6 — Controller-Friendly Overlay Gallery
Console-feel gallery: big preview, thumb strip, full pad mapping (X/Circle/Triangle/Square/L1/R1/Options), nav + feedback sounds.
**Accept:** fully mouse-free · browse/play/favorite/delete from pad · couch-readable · sounds subtle.

## 0.7 — Storage & Cleanup
Limits, auto-cleanup, favorite protection, disk warnings, manual cleanup screen, per-game usage.
**Accept:** limit respected, oldest non-favorites deleted, favorites untouchable, per-game view, imports never modified without opt-in.

## 1.0 — Polished Release
Quality presets, per-game profiles, external imports (Game Bar/Steam/NVIDIA/OBS), advanced settings, reliable tray, autostart, portable mode, sound packs, custom bindings UI, notifications, crash-safe logging, settings export/import, themes.
