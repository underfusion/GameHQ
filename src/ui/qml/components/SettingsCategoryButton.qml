import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic as QC
import GameHQ

QC.ItemDelegate {
    id: root
    property string glyph: ""
    property string label: ""
    property bool selected: false
    implicitHeight: Theme.s48
    focusPolicy: Qt.StrongFocus

    contentItem: RowLayout {
        spacing: Theme.s8
        Text {
            visible: root.glyph.length > 0
            text: root.glyph
            color: root.selected || root.activeFocus ? Theme.accent : Theme.textFaint
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontBody
            font.weight: Font.DemiBold
            Layout.preferredWidth: Theme.s24
            horizontalAlignment: Text.AlignHCenter
        }
        Text {
            text: root.label
            color: root.selected || root.activeFocus ? Theme.text : Theme.textMuted
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontBody
            font.weight: root.selected ? Font.DemiBold : Font.Normal
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
            Layout.fillWidth: true
        }
    }

    background: Rectangle {
        radius: Theme.radiusM
        color: root.selected ? Theme.surfaceElevated
                             : (root.hovered || root.activeFocus ? Theme.surfaceHover : "transparent")
        border.width: root.activeFocus ? 1 : 0
        border.color: Theme.focusRing
        Rectangle {
            visible: root.selected
            width: Theme.s4
            height: parent.height - Theme.s16
            radius: Theme.radiusPill
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            color: Theme.accent
        }
    }
}
