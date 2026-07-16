import QtQuick

// Base for every skin: declares the full color surface with the Dark values as
// defaults. A skin overrides only what it changes, so a token a skin forgets
// falls back to Dark rather than resolving to an invalid color.
//
// Only COLORS live here. Sizes, fonts, spacing, radii and durations are layout
// and behavior, not palette — they stay fixed in Theme.qml across every skin.
//
// Adding a token: declare it here first, then in Theme.qml. Never name one
// `onAccent` (or any `on<Existing>`) — QML parses that as a signal handler for
// the matching property and refuses to load. See docs/design-system.md.
QtObject {
    // Display name shown in Settings.
    property string label: "Dark"

    // Surfaces
    property color bg0:        "#0B0F20"
    property color bg1:        "#131A33"
    property color surface:    "#171F3D"
    property color surfaceAlt: "#1D2749"
    property color stroke:     Qt.rgba(1, 1, 1, 0.06)
    property color borderLight: Qt.rgba(1, 1, 1, 0.22)
    property color panelTint:  Qt.rgba(0.0902, 0.1216, 0.2392, 0.82)

    // Brand accents
    property color accent1: "#8B6BFF"
    property color accent2: "#3FA9FF"
    property color accent:  "#6D8DFF"

    // Text
    property color text:      "#E8EAF0"
    property color textMuted: "#8B93A7"
    property color textFaint: "#5A6378"
    property color textOnAccent: "#FFFFFF"

    // Semantic
    property color danger:  "#FF5D73"
    property color success: "#4ADE80"
    property color warning: "#FFC24D"

    // Scrims sit behind media (lightbox, overlay) and stay dark in every skin —
    // a light scrim would wash out the capture it is meant to frame.
    property color scrim:         Qt.rgba(0.043, 0.059, 0.125, 0.6)
    property color lightboxScrim: Qt.rgba(0.043, 0.059, 0.125, 0.74)
    property color focusGlow:     Qt.rgba(0.427, 0.553, 1, 0.35)

    // Hover/press wash. `highlight` is the color, the opacity is the caller's —
    // so a light skin flips this to black rather than changing every call site.
    property color highlight: "#FFFFFF"
    property color hoverTint: Qt.rgba(1, 1, 1, 0.03)

    // Chrome drawn over video frames (play badge, tile buttons, player pulse)
    // must read on arbitrary imagery, so it stays dark in every skin.
    property color badgeFill:   Qt.rgba(0, 0, 0, 0.40)
    property color badgeBorder: Qt.rgba(1, 1, 1, 0.9)
    property color badgeGlyph:  Qt.rgba(1, 1, 1, 0.95)
    property color tileButtonIdle:  Qt.rgba(0, 0, 0, 0.55)
    property color tileButtonHover: Qt.rgba(0, 0, 0, 0.8)
    property color pulseFill: Qt.rgba(0, 0, 0, 0.46)

    // Quiet AccentButton variants
    property color dangerQuietBorder:  Qt.rgba(1.0, 0.36, 0.45, 0.45)
    property color dangerQuietTop:     "#301B28"
    property color dangerQuietBottom:  "#21141E"
    property color successQuietBorder: Qt.rgba(0.29, 0.87, 0.50, 0.45)
    property color successQuietTop:    "#173328"
    property color successQuietBottom: "#11251E"
}
