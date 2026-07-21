import QtQuick
import QtQuick.Effects
import GameHQ

// Sidebar navigation entry — pill highlight + 3 px accent bar when active.
// `sidebarHovered` lights a 2px accent border when the cursor has been moved
// INTO the sidebar (L1 from the pad — the only entry path); this is a distinct
// visual from the steady `active` state, so the user always sees which sidebar
// row the cursor is on while the grid sits unfocused to its right.
Rectangle {
    id: root
    property string label
    property string glyph: ""          // small unicode glyph stands in for icons pre-1.0
    property string iconSource: ""
    property string trailingText: ""
    property string trailingGlyph: ""
    property color trailingGlyphColor: Theme.accent
    readonly property real gameIconSize: Theme.s12 * 1.2
    property bool active: false
    property bool sidebarHovered: false
    signal clicked()

    width: parent ? parent.width : 200
    height: 40
    radius: Theme.radiusS
    color: active || sidebarHovered ? Theme.surfaceAlt
         : mouse.containsMouse || root.activeFocus ? Theme.hoverTint
         : "transparent"

    // No color Behavior: it animated the active-tab highlight too, so
    // switching tabs (especially via L1/R1 pad quick-step) left the
    // previously-active item visibly fading its highlight out for ~140ms
    // instead of clearing immediately.

    activeFocusOnTab: true
    Keys.onReturnPressed: root.clicked()
    Keys.onSpacePressed: root.clicked()

    Rectangle { // active accent bar
        visible: root.active
        width: 3; height: 18; radius: 2
        anchors.verticalCenter: parent.verticalCenter
        color: Theme.accent
    }

    Row {
        id: contentRow
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: Theme.s16
        anchors.rightMargin: Theme.s16
        spacing: Theme.s12

        Rectangle {
            id: gameIconFrame
            visible: root.iconSource !== ""
            width: root.gameIconSize
            height: root.gameIconSize
            radius: Theme.s4
            color: "transparent"
            anchors.verticalCenter: parent.verticalCenter

            Item {
                id: gameIconLayer
                anchors.fill: parent
                layer.enabled: true
                layer.effect: MultiEffect {
                    maskEnabled: true
                    maskSource: gameIconMask
                }

                Image {
                    anchors.fill: parent
                    source: root.iconSource
                    sourceSize.width: root.gameIconSize
                    sourceSize.height: root.gameIconSize
                    fillMode: Image.PreserveAspectCrop
                    smooth: true
                }
            }

            Rectangle {
                id: gameIconMask
                anchors.fill: gameIconLayer
                radius: gameIconFrame.radius
                visible: false
                layer.enabled: true
            }
        }

        Text {
            text: root.glyph
            visible: root.glyph !== "" && root.iconSource === ""
            color: root.active ? Theme.accent : Theme.textMuted
            font.pixelSize: Theme.fontBody
            anchors.verticalCenter: parent.verticalCenter
        }
        Text {
            text: root.label
            width: Math.max(0, contentRow.width
                            - (root.iconSource !== "" ? root.gameIconSize + contentRow.spacing
                               : root.glyph !== "" ? Theme.s24 + contentRow.spacing
                               : 0)
                            - (trailing.visible ? trailing.implicitWidth + contentRow.spacing : 0))
            elide: Text.ElideRight
            color: root.active ? Theme.text : Theme.textMuted
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontBody
            anchors.verticalCenter: parent.verticalCenter
        }

        Row {
            id: trailing
            visible: root.trailingText !== "" || root.trailingGlyph !== ""
            spacing: Theme.s8
            anchors.verticalCenter: parent.verticalCenter

            Text {
                visible: root.trailingText !== ""
                text: root.trailingText
                color: Theme.textFaint
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontCaption
                anchors.verticalCenter: parent.verticalCenter
            }
            Text {
                visible: root.trailingGlyph !== ""
                text: root.trailingGlyph
                color: root.trailingGlyphColor
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontCaption
                font.weight: Font.DemiBold
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }

    // Focus ring — also lights when the cursor has been moved INTO the sidebar
    // (sidebarHovered) so the highlighted row is unambiguous even though no
    // QML item actually holds activeFocus while the grid keeps keyboard focus.
    Rectangle {
        anchors.fill: parent
        radius: parent.radius
        color: "transparent"
        border.width: (root.activeFocus || root.sidebarHovered) ? 2 : 0
        border.color: Theme.accent
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        onClicked: root.clicked()
    }
}
