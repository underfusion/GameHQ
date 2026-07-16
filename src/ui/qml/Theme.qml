pragma Singleton
import QtQuick

// Single source of truth for all visual values — see docs/design-system.md.
// Components must never hardcode colors, sizes, or durations.
QtObject {
    // Color tokens
    readonly property color bg0:        "#0B0F20"
    readonly property color bg1:        "#131A33"
    readonly property color surface:    "#171F3D"
    readonly property color surfaceAlt: "#1D2749"
    readonly property color stroke:     Qt.rgba(1, 1, 1, 0.06)
    readonly property color borderLight: Qt.rgba(1, 1, 1, 0.22)   // visible light outline (e.g. capture previews)
    readonly property color accent1:    "#8B6BFF"
    readonly property color accent2:    "#3FA9FF"
    readonly property color accent:     "#6D8DFF"
    readonly property color text:       "#E8EAF0"
    readonly property color textMuted:  "#8B93A7"
    readonly property color textFaint:  "#5A6378"
    readonly property color danger:     "#FF5D73"
    readonly property color success:    "#4ADE80"
    readonly property color warning:    "#FFC24D"
    readonly property color scrim:      Qt.rgba(0.043, 0.059, 0.125, 0.6)
    readonly property color lightboxScrim: Qt.rgba(0.043, 0.059, 0.125, 0.74)
    readonly property color focusGlow:  Qt.rgba(0.427, 0.553, 1, 0.35)
    // Translucent surface for in-overlay panels (sidebar, strip, preview frame)
    // — sits over the scrim with a hairline stroke so panels read as distinct
    // floating layers, not painted on the backdrop. Derived from `surface`
    // (not a separate lighter blue) so overlay panels match the app's actual
    // dark surface tone instead of looking washed out.
    readonly property color panelTint:  Qt.rgba(0.0902, 0.1216, 0.2392, 0.82)

    // Foreground on the accent gradient — stays white on every accent tint,
    // so it is its own token rather than a `text` variant. NOT named `onAccent`:
    // QML would parse that as a signal handler for the `accent` property above
    // and refuse to load the singleton.
    readonly property color textOnAccent: "#FFFFFF"
    // White wash laid over a control for hover/press feedback; the opacity is
    // the caller's, the color is always this.
    readonly property color highlight:  "#FFFFFF"
    // Faint hover fill for list rows that have no surface of their own.
    readonly property color hoverTint:  Qt.rgba(1, 1, 1, 0.03)

    // Video play badge — the circular ▶ marker drawn over a video thumbnail
    // (capture tile, overlay preview, toast). Reads on any frame, so it is
    // deliberately independent of the surface palette.
    readonly property color badgeFill:   Qt.rgba(0, 0, 0, 0.40)
    readonly property color badgeBorder: Qt.rgba(1, 1, 1, 0.9)
    readonly property color badgeGlyph:  Qt.rgba(1, 1, 1, 0.95)

    // Circular icon button floating over a thumbnail — darker than the badge
    // so its glyph stays legible without a border.
    readonly property color tileButtonIdle:  Qt.rgba(0, 0, 0, 0.55)
    readonly property color tileButtonHover: Qt.rgba(0, 0, 0, 0.8)

    // Player's centered play/pause pulse.
    readonly property color pulseFill: Qt.rgba(0, 0, 0, 0.46)

    // Quiet (low-emphasis) AccentButton tinted to a semantic color: a muted
    // gradient pair plus a resting border derived from the same hue. Only the
    // danger and success variants exist because only those are used.
    readonly property color dangerQuietBorder:  Qt.rgba(1.0, 0.36, 0.45, 0.45)
    readonly property color dangerQuietTop:     "#301B28"
    readonly property color dangerQuietBottom:  "#21141E"
    readonly property color successQuietBorder: Qt.rgba(0.29, 0.87, 0.50, 0.45)
    readonly property color successQuietTop:    "#173328"
    readonly property color successQuietBottom: "#11251E"

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
