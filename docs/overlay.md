# Overlay Design

> Milestone 0.2. Core rule: **GameHQ is primarily an in-game overlay**, never a window you alt-tab into.

## Behavior contract

- PS (or hotkey `Ctrl+Shift+G`, or Share double-tap fallback): toggle overlay.
- On open: the first sidebar tab is `Game`, filtered to screenshots and clips for the game that had focus before the overlay appeared. When a current game is available, `Game Favourites` appears directly below it and shows only favourite captures from that same game.
- Circle: back in submenus/viewer; close from main gallery.
- On close: hide → restore focus to remembered game HWND → controller input returns to game.
- Overlay open ⇒ GameHQ consumes all controller input (game must not react).
- Any OS-level focus change away from the overlay (Windows key, Alt-Tab, task
  switcher, clicking another window) auto-hides the overlay the same way
  Circle/click-outside does, **except** it does not force focus back onto the
  remembered game — that would fight whatever just took focus (e.g. yanking
  focus away from a freshly-opened Start menu). Detected process-wide via
  `SetWinEventHook(EVENT_SYSTEM_FOREGROUND)` in `OverlayManager` so every
  system focus-changing operation is covered, not just a hard-coded key list.

## Window technique (MVP, no injection)

- Qt `QQuickWindow` with `Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint`, no taskbar entry (`Qt::Tool`).
- On show: remember `GetForegroundWindow()` (the game), then `SetForegroundWindow` on the overlay; keep topmost with `SetWindowPos(HWND_TOPMOST)`. The plain `SetForegroundWindow` was rejected by foreground-lock when the game owned focus; the show path now goes through `OverlayManager`'s `forceForeground()` helper, which uses the `AttachThreadInput` trick to bypass the lock (verified by HWND-before/after logging in `gamehq.log`).
- On hide: focus is restored to the remembered game HWND via the same `forceForeground()` path — `AttachThreadInput` is now actually implemented on both sides (was a documented "may require" TODO).
- Supported: borderless fullscreen, windowed fullscreen, windowed. **Exclusive fullscreen not guaranteed** — detect (window covers monitor + `IsIconic` false + swap-chain heuristics unavailable without injection) and advise switching to borderless.
- **Never** inject into game processes (anti-cheat).

## QML structure

- `OverlayWindow.qml` owns overlay state, keyboard/controller routing, preview playback state, and top-level layout.
- `components/OverlaySidebar.qml` renders category/game navigation and a
  centered brand lockup matching the desktop sidebar's icon and typography,
  with the running app version printed underneath it.
- `components/OverlayCaptureStrip.qml` renders the horizontal capture strip and L1/R1 or arrow hint pills. While video focus is inactive, controller/keyboard left-right navigation can browse captures; once Cross enters video focus, left-right seeks the clip and L1/R1 remains the capture-switch path. Each tile's hover icons are also mouse-clickable: heart toggles favourite, folder reveals the capture on disk, and trash opens `OverlayWindow.qml`'s "Delete capture?" confirmation — the strip only moves selection to the clicked tile and forwards the action, `OverlayWindow` owns the actual delete/folder/favourite calls.
- `components/OverlayPreview.qml` renders the large selected-capture preview. It reacts to overlay gallery model resets/row insert-remove-move/data changes as well as index changes, so a newly saved screenshot or clip at row 0 updates the preview immediately when the overlay opens. Because `galleryModel.get()` is an imperative call that no model signal re-evaluates on its own, *every* binding that resolves a record — both the target URL and the displayed record that drives play/badge decisions — must read the `_modelRevision` counter. Omitting it on the record binding is what made a just-saved clip refuse to play on Cross and a just-saved screenshot inherit the previous clip's play badge (fixed in 0.6.2). The still/clip surface itself is the shared `components/MediaStage.qml`; the preview supplies the overlay's own rules for it (a clip decodes its thumbnail, which keeps painting until `videoFocused` hands the stage to the video surface) and tracks the committed frame against the requested one via the stage's `committed`/`cleared` signals.
- `components/OverlayFooter.qml` renders contextual footer hints.
- `components/OverlayActionMenu.qml` renders per-capture actions and is also reused by the desktop pad action menu.

## Known risks

Focus-restoration flakiness · Steam Input remapping the pad while overlay focused · games reading input via Raw Input regardless of focus (log + document per game) · multi-monitor placement (show on the game's monitor).
