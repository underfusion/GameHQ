import QtQuick
import GameHQ

// Small round icon button for capture-tile hover actions. Glyphs come from the
// Windows "Segoe Fluent Icons" system font. Reports hover via `hovered` and a
// clicked() signal.
Rectangle {
    id: root

    property string glyph: ""
    property color glyphColor: Theme.text
    property real diameter: 30
    property alias hovered: mouse.containsMouse
    signal clicked()

    implicitWidth: diameter
    implicitHeight: diameter
    width: diameter
    height: diameter
    radius: diameter / 2
    color: mouse.containsMouse ? Qt.rgba(0, 0, 0, 0.8) : Qt.rgba(0, 0, 0, 0.55)
    border.width: 1
    border.color: Theme.stroke

    scale: mouse.containsMouse ? 1.08 : 1.0
    Behavior on scale { NumberAnimation { duration: Theme.durFast; easing.type: Easing.OutCubic } }

    Text {
        anchors.centerIn: parent
        text: root.glyph
        color: root.glyphColor
        font.family: "Segoe Fluent Icons"
        font.pixelSize: Math.round(root.diameter * 0.48)
    }

    // MouseArea, not HoverHandler+TapHandler: a tap here also landed on
    // CaptureTile's own TapHandler underneath (opening the lightbox at the
    // same time as delete/open-folder/favorite fired) — TapHandler doesn't
    // reliably block a same-point tap from also reaching an ancestor's
    // TapHandler. MouseArea's press grab does (same technique already used
    // elsewhere in this app — Lightbox, ConfirmDialog — to swallow clicks).
    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        onClicked: root.clicked()
    }
}
