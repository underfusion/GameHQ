import QtQuick
import GameHQ

// The circular ▶ marker drawn over a video thumbnail, shared by the capture
// tile, the overlay preview and the toast. Diameter is the only thing that
// varies between them — callers derive it from their own thumbnail's smaller
// side and set `diameter`; everything else (ring, glyph, proportions) is fixed
// here so the three surfaces cannot drift apart again.
//
// Callers own `visible` and their own anchoring.
Rectangle {
    id: root
    // Circle diameter, in px. Named like TileIconButton's for consistency.
    property real diameter: 0

    width: diameter
    height: diameter
    radius: width / 2
    color: Theme.badgeFill
    border.width: Math.max(2, width * 0.035)
    border.color: Theme.badgeBorder
    opacity: 0.78                     // slightly transparent overall

    Text {
        anchors.centerIn: parent
        // The glyph carries its own whitespace on the left, so centering it
        // geometrically looks off — nudge right to visually center.
        anchors.horizontalCenterOffset: parent.width * 0.03
        text: "▶"
        color: Theme.badgeGlyph
        font.pixelSize: parent.width * 0.63
    }
}
