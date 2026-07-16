# Design System

> **Rule: every visual value used in the app comes from `src/ui/qml/Theme.qml` (singleton). No hardcoded colors, sizes, or durations in components — ever.** This doc is the human-readable contract; Theme.qml is the machine-readable one. Update both together.

## Direction

Modern, minimalist, **PS5-inspired**: dark, spacious, content-first. The captures *are* the interface — chrome stays quiet, typography is light and airy, and a single violet→blue accent (from the app icon) does all the talking. Everything must look correct from couch distance and navigate beautifully with a controller.

## 0. Skins

Colors resolve through the **active skin**; everything else in `Theme.qml`
(typography, spacing, radii, motion, player timings) is fixed across skins. That
split is deliberate: a skin recolors the app, it does not re-lay-it-out.

- `themes/Skin.qml` declares the whole color surface with the **Dark values as
  defaults**. `DarkSkin` overrides nothing, `LightSkin` and `HighContrastSkin`
  override only what differs — so a token a skin forgets falls back to Dark
  instead of resolving to an invalid color. Add a token here first, then bind it
  in `Theme.qml`.
- `Theme.activeSkin` is seeded from `theme.active_skin` (`dark` | `light` |
  `high_contrast`) and re-assigned on `AppController::configChanged`, which
  re-evaluates every color binding — that is what makes the switch live. An
  unknown value resolves to Dark.
- **Never name a token `on<Existing>`** (e.g. `onAccent` next to `accent`): QML
  parses it as a signal handler and refuses to load the singleton, taking the
  whole UI down. It is `textOnAccent` for exactly this reason.
- Components are unaffected by all of this — every token name is unchanged, and
  nothing outside `Theme.qml`/`themes/` knows skins exist. Keep it that way.

Values in the table below are the **Dark** (default) skin's.

## 1. Color tokens

| Token | Value | Use |
|---|---|---|
| `bg0` | `#0B0F20` | window background (bottom of gradient) |
| `bg1` | `#131A33` | window background (top of gradient) |
| `surface` | `#171F3D` | cards, sidebar, panels |
| `surfaceAlt` | `#1D2749` | hover state, nested surfaces |
| `stroke` | `#FFFFFF` @ 6 % | hairline borders on surfaces |
| `accent1` | `#8B6BFF` | gradient start (violet) |
| `accent2` | `#3FA9FF` | gradient end (blue) |
| `accent` | `#6D8DFF` | solid accent (focus, links, active states) |
| `text` | `#E8EAF0` | primary text |
| `textMuted` | `#8B93A7` | secondary text, captions |
| `textFaint` | `#5A6378` | disabled, placeholders |
| `danger` | `#FF5D73` | destructive actions, record dot |
| `success` | `#4ADE80` | confirmations |
| `warning` | `#FFC24D` | warnings |
| `textOnAccent` | `#FFFFFF` | text/icons on the accent gradient (never `onAccent` — QML reads `on…` as a signal handler) |
| `highlight` | `#FFFFFF` | white wash for hover/press feedback; the caller owns the opacity |
| `hoverTint` | `#FFFFFF` @ 3 % | hover fill for rows with no surface of their own |
| `badgeFill` | black @ 40 % | video play-badge circle |
| `badgeBorder` | `#FFFFFF` @ 90 % | video play-badge outline |
| `badgeGlyph` | `#FFFFFF` @ 95 % | video play-badge ▶ |
| `tileButtonIdle` | black @ 55 % | circular icon button over a thumbnail |
| `tileButtonHover` | black @ 80 % | same, hovered |
| `pulseFill` | black @ 46 % | player's centered play/pause pulse |
| `dangerQuietTop` / `dangerQuietBottom` / `dangerQuietBorder` | `#301B28` / `#21141E` / `danger` @ 45 % | quiet AccentButton tinted destructive |
| `successQuietTop` / `successQuietBottom` / `successQuietBorder` | `#173328` / `#11251E` / `success` @ 45 % | quiet AccentButton tinted confirm |

The badge and tile-button tokens are deliberately neutral black/white rather than palette hues: they sit on arbitrary video frames and must stay legible against any content.

Rules: one accent family only — never introduce new hues for decoration. Gradient (`accent1→accent2`, 135°) is reserved for: primary buttons, active selection fills, progress, and brand marks. Backgrounds are always the `bg1→bg0` vertical gradient, never flat black.

## 2. Typography

Font: **Segoe UI Variable Display**, fallback Segoe UI (system-native, ships with Win11/10).

| Token | Size / weight | Use |
|---|---|---|
| `fontHero` | 48 px | oversized decorative glyph in empty states |
| `fontDisplay` | 32 px / Light | screen titles (PS5-style thin headers) |
| `fontTitle` | 22 px / DemiBold | section titles, dialog headers |
| `fontH3` | 16 px / DemiBold | card titles, sidebar group labels |
| `fontBody` | 14 px / Regular | default text |
| `fontCaption` | 12 px / Regular | metadata, timestamps, hints |

Rules: big numbers/headers are Light, never Bold. No ALL-CAPS except tiny group labels (`letterSpacingWide`, +1). Line height ≥ 1.4 for body.

## 3. Spacing, radius, elevation

- Spacing scale (4-base): `4, 8, 12, 16, 24, 32, 48`. Nothing off-scale.
- Radius: `radiusS 8` (inputs, chips) · `radiusM 12` (cards, tiles) · `radiusL 16` (panels, dialogs) · `radiusPill 999`.
- Elevation = subtle 6 % white hairline + soft black shadow (no bright borders). Max two elevation levels on screen.
- Borders: `stroke` (6 %) outlines a **panel/container**; `borderLight` (22 %) outlines a **field inside** one, and is also the token for a 1 px row divider. Don't mix the two on the same kind of element.

## 4. Focus & controller navigation (the PS5 signature)

- **Every interactive element has a visible focus state** — a 2 px `accent` ring plus outer glow (`accent` @ 35 %, 12 px blur).
- Focused gallery tiles **scale to 1.04** (220 ms OutCubic) — the PS5 tile zoom. Neighbors do not move (scale within own bounds).
- Focus is never trapped; D-pad/arrow order is always predictable (left↔right within a row, up↓down across rows/sections).
- Mouse hover mirrors focus styling at ~60 % intensity; hover never reveals actions that focus can't reach. In the main-app gallery, hovering a tile also moves the keyboard cursor to it (`grid.currentIndex` follows the mouse), so the accent frame + focus zoom appear on hover exactly as on keyboard navigation (dev.69).

## 5. Motion

| Token | Value | Use |
|---|---|---|
| `durFast` | 140 ms | hovers, presses, toggles |
| `durNormal` | 220 ms | focus scale, panel slides, fades |
| `durSlow` | 320 ms | overlay open/close |
| `playerSeekStepMs` | 2000 ms | left/right video seek step |
| `playerHudHoldMs` | 1600 ms | centered play/pause pulse before fade-out |
| `playerControlsAutoHideMs` | 5000 ms | video controls inactivity timeout |

Easing: OutCubic everywhere (OutQuint for overlay slide). Rules: animate opacity + transform only (no layout thrash); no bounces or overshoots; motion always has a purpose (orientation, feedback) — never decoration.

## 6. Components

- **Desktop gallery shell**: `components/DesktopSidebar.qml`, `components/DesktopGalleryHeader.qml`, and `components/DesktopGalleryFooter.qml` own the desktop navigation chrome, title/bulk actions, and hint/zoom controls. Keep these presentational controls out of `Main.qml`; `Main.qml` should own window state and behavior-heavy grid navigation.
- **Sidebar item**: 40 px tall, pill highlight `surfaceAlt` when active + 3 px accent bar on the left; game rows may show a cached executable icon at `Theme.s12 * 1.2` with a real rounded image mask before the title; category/settings rows use the existing glyph fallback. Labels are `fontBody`, muted → text on active, and elide rather than overflow.
- **Capture tile**: 16:9 thumbnail, `radiusM`, hairline stroke; video = centered play glyph @ 70 % white over a darkened thumb; caption below tile (game · date, `fontCaption`, muted). In the desktop gallery, the left edge of the first visible tile matches the normal inter-tile gap from the sidebar. The top-right `Bulk Select` action uses the standard bordered `AccentButton` with an icon and enters multi-select mode; Select mode actions (`Select all` / `Deselect all`, `Delete`, `Done`) are also icon+label buttons. **Hover actions** (quick `durFast` fade): round icon buttons (`TileIconButton`, Segoe Fluent Icons) — **delete** (`danger`) + **open-folder** bottom-left, **favourite heart** bottom-right; a favourited tile keeps its filled heart visible without hover. Delete opens a modal `ConfirmDialog`. In main-app Select mode, hover actions are hidden and every tile shows a top-left square checkbox; tapping/cross toggles it without dimming the thumbnail grid.
- **Video badge**: `components/VideoBadge.qml` — the circular ▶ marker that flags a video thumbnail, shared by the capture tile, the overlay preview and the toast. Callers set only `diameter` (derived from their thumbnail's smaller side) plus `visible` and their own anchoring; the ring, glyph and proportions live in the component so the three surfaces can't drift apart. Never re-draw this badge inline.
- **Video player controls**: bottom-centered controls span the preview width without a backing panel, with a pill progress track, circular buttons (`playerButtonSize`) and `fontCaption` time text. Left/right seek by `playerSeekStepMs`. It auto-hides after `playerControlsAutoHideMs` and reappears on keyboard/controller/mouse input. Clicking the video surface toggles play/pause; right-click closes the active lightbox/focused preview. Finished clips pause on the final frame and stay on the current capture. The center play/pause pulse draws pause as two thick rounded bars and play as a drawn triangle, fades after `playerHudHoldMs`, and must remain usable by both controller and mouse.
- **Lightbox controls**: desktop Lightbox uses the darker `lightboxScrim` backdrop. Its white pill controls (`radiusPill`, `text` background, `bg0` glyph/text) sit on the preview edges: left/right pills are 48 px tall clickable mouse targets that step to previous/next capture and show `L1` / `R1` after gamepad input or `←` / `→` after keyboard input. A matching top-right circular close pill closes the viewer. L1/R1 always steps to the next/previous capture (regardless of type); d-pad left/right inside a video preview still seeks.
- **Overlay strip hint pills**: in the **overlay** captures strip panel, two smaller white pill labels (half the Lightbox size — `s16` height, `fontCaption` text, `-s12` outside the panel edges, vertically centered) always show `L1` / `R1` — the overlay's pad navigation is bumpers-only for flipping captures (d-pad ←/→ is a no-op in the overlay).
- **Current game sidebar section**: when a focused game has captures, `Game` and `Game Favourites` categories appear above `All` in both the desktop and overlay sidebars. `Game` filters to that game's screenshots and clips; `Game Favourites` filters to favourite captures from that same game. When no game is focused, or the focused game has no captures yet, both categories are hidden and `All` remains the first category.
- **Primary button**: gradient fill, white text, `radiusS`, 36 px tall. **Secondary**: `surface` + hairline. **Ghost**: text-only, accent on hover/focus. Destructive: `danger` text or fill for confirm step only.
- **Dialogs/popups**: `surface`, `radiusL`, dim scrim `bg0` @ 60 %; one primary action max.
- **Settings shell**: a compact `bg1` category rail sits beside one scrollable page; Input may add nested device tabs, but top-level categories never grow into one long page.
- **Settings controls**: compose `SettingsPage`, `SettingsSection`, `SettingsRow`, `SettingsToggle`, `SettingsCategoryButton`, `SettingsCombo`, and `SettingsPathField` (the read-only capture-root box — set `text`, it owns its own outline and middle elision); changed/reset state must remain observable and all visuals come from `Theme.qml`.
- **Empty states**: centered icon (muted), one sentence, one action — never a blank grid.

## 7. Sounds (paired with motion)

UI sounds and animations fire together or not at all (see [sound-system.md](sound-system.md)): nav tick ↔ focus move · favorite ↔ heart pop · overlay open/close ↔ slide. Volume subtle; motion works standalone when sounds are off.

## 8. Do / Don't

- ✅ Dark always (no light theme before 1.0 theming) · ✅ content-first: thumbnails largest thing on screen · ✅ one accent · ✅ every state reachable by controller.
- ❌ No pure black `#000` or pure white `#FFF` surfaces · ❌ no borders brighter than 10 % white · ❌ no more than one gradient element per view region · ❌ no text below 12 px · ❌ no spinner where a skeleton/fade suffices.
