import QtQuick
import QtQuick.Controls.Basic as QC
import GameHQ

QC.ItemDelegate {
    id: root
    property string label: ""
    property bool selected: false
    implicitHeight: Theme.s48
    focusPolicy: Qt.StrongFocus
    contentItem: Text {
        text: root.label
        color: root.selected || root.activeFocus ? Theme.text : Theme.textMuted
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontBody
        font.weight: root.selected ? Font.DemiBold : Font.Normal
        verticalAlignment: Text.AlignVCenter
        leftPadding: Theme.s12
        elide: Text.ElideRight
    }
    background: Rectangle {
        radius: Theme.radiusM
        color: root.selected ? Theme.surfaceAlt : (root.hovered || root.activeFocus ? Theme.surface : "transparent")
        border.width: root.activeFocus ? 1 : 0
        border.color: Theme.accent
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
