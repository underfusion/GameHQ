pragma Singleton
import QtQuick
import "themes"

// Single source of truth for all visual values — see docs/design-system.md.
// Components must never hardcode colors, sizes, or durations.
//
// Colors resolve through the active skin (themes/Skin.qml and its variants);
// everything below the color block is fixed across skins because it is layout
// and behavior, not palette. Components are unaffected either way: every token
// name here is unchanged, so nothing outside this file knows skins exist.
QtObject {
    id: theme

    // ───────────────────────── Skin selection ─────────────────────────
    readonly property var availableSkins: [
        { key: "dark",          label: darkSkin.label },
        { key: "light",         label: lightSkin.label },
        { key: "high_contrast", label: highContrastSkin.label }
    ]

    // Persisted as `theme.active_skin`. Assigned (not bound) so the Settings
    // combo can flip it live; seeded from config once the engine is up.
    property string activeSkin: "dark"

    readonly property Skin darkSkin: DarkSkin {}
    readonly property Skin lightSkin: LightSkin {}
    readonly property Skin highContrastSkin: HighContrastSkin {}

    // Unknown key → Dark, so a hand-edited config.json cannot leave the app
    // with no palette at all.
    readonly property Skin skin: theme.activeSkin === "light" ? lightSkin
        : theme.activeSkin === "high_contrast" ? highContrastSkin
        : darkSkin

    Component.onCompleted: {
        if (typeof app !== "undefined" && app)
            theme.activeSkin = app.config("theme.active_skin", "dark")
    }

    // Repaint live when the skin is changed from Settings — every color token
    // below binds to `skin`, so reassigning it re-evaluates all of them.
    readonly property Connections _configWatch: Connections {
        target: (typeof app !== "undefined") ? app : null
        function onConfigChanged(key, value) {
            if (key === "theme.active_skin")
                theme.activeSkin = value
        }
    }

    // ───────────────────────── Color tokens ─────────────────────────
    // Skinnable. Defaults and per-skin overrides live in themes/.
    readonly property color bg0:        skin.bg0
    readonly property color bg1:        skin.bg1
    readonly property color surface:    skin.surface
    readonly property color surfaceAlt: skin.surfaceAlt
    readonly property color stroke:     skin.stroke
    readonly property color borderLight: skin.borderLight   // visible light outline (e.g. capture previews)
    readonly property color accent1:    skin.accent1
    readonly property color accent2:    skin.accent2
    readonly property color accent:     skin.accent
    readonly property color text:       skin.text
    readonly property color textMuted:  skin.textMuted
    readonly property color textFaint:  skin.textFaint
    readonly property color danger:     skin.danger
    readonly property color success:    skin.success
    readonly property color warning:    skin.warning
    readonly property color scrim:      skin.scrim
    readonly property color lightboxScrim: skin.lightboxScrim
    readonly property color focusGlow:  skin.focusGlow
    // Translucent surface for in-overlay panels (sidebar, strip, preview frame)
    // — sits over the scrim with a hairline stroke so panels read as distinct
    // floating layers, not painted on the backdrop.
    readonly property color panelTint:  skin.panelTint

    // Foreground on the accent gradient — its own token rather than a `text`
    // variant, since it tracks the accent, not the body text. NOT named
    // `onAccent`: QML would parse that as a signal handler for the `accent`
    // property above and refuse to load the singleton.
    readonly property color textOnAccent: skin.textOnAccent
    // Wash laid over a control for hover/press feedback; the opacity is the
    // caller's, the color is the skin's (white on dark, black on light).
    readonly property color highlight:  skin.highlight
    // Faint hover fill for list rows that have no surface of their own.
    readonly property color hoverTint:  skin.hoverTint

    // Video play badge — the circular ▶ marker drawn over a video thumbnail
    // (capture tile, overlay preview, toast). Reads on any frame, so it is
    // deliberately independent of the surface palette.
    readonly property color badgeFill:   skin.badgeFill
    readonly property color badgeBorder: skin.badgeBorder
    readonly property color badgeGlyph:  skin.badgeGlyph

    // Circular icon button floating over a thumbnail — darker than the badge
    // so its glyph stays legible without a border.
    readonly property color tileButtonIdle:  skin.tileButtonIdle
    readonly property color tileButtonHover: skin.tileButtonHover

    // Player's centered play/pause pulse.
    readonly property color pulseFill: skin.pulseFill

    // Quiet (low-emphasis) AccentButton tinted to a semantic color: a muted
    // gradient pair plus a resting border derived from the same hue. Only the
    // danger and success variants exist because only those are used.
    readonly property color dangerQuietBorder:  skin.dangerQuietBorder
    readonly property color dangerQuietTop:     skin.dangerQuietTop
    readonly property color dangerQuietBottom:  skin.dangerQuietBottom
    readonly property color successQuietBorder: skin.successQuietBorder
    readonly property color successQuietTop:    skin.successQuietTop
    readonly property color successQuietBottom: skin.successQuietBottom

    // ── Everything below is fixed across skins: layout and behavior. ──

    // Typography
    readonly property string fontFamily: "Segoe UI Variable Display"
    readonly property int fontHero:    48   // oversized glyph in empty states
    readonly property int fontDisplay: 32   // weight Light
    readonly property int fontTitle:   22   // weight DemiBold
    readonly property int fontH3:      16   // weight DemiBold
    readonly property int fontBody:    14
    readonly property int fontCaption: 12
    // Tracking for the all-caps section labels.
    readonly property real letterSpacingWide: 1

    // Spacing (4-base scale)
    readonly property int s4:  4
    readonly property int s8:  8
    readonly property int s12: 12
    readonly property int s16: 16
    readonly property int s24: 24
    readonly property int s32: 32
    readonly property int s48: 48

    // Video player controls
    readonly property int playerButtonSize: 44
    readonly property int playerControlsMaxWidth: 9999
    readonly property int playerSeekStepMs: 2000
    readonly property int playerHudHoldMs: 1600
    readonly property int playerControlsAutoHideMs: 5000

    // Radius
    readonly property int radiusS: 8
    readonly property int radiusM: 12
    readonly property int radiusL: 16
    readonly property int radiusPill: 999

    // Motion
    readonly property int durFast:   140
    readonly property int durNormal: 220
    readonly property int durSlow:   320
    readonly property real focusScale: 1.04
}
