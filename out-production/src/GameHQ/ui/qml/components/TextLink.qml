import QtQuick
import QtQuick.Layouts
import GameHQ

Item {
    id: root

    property string label
    property string suffix: ""
    signal clicked()

    implicitWidth: linkRow.implicitWidth
    implicitHeight: 30
    Layout.preferredWidth: implicitWidth
    Layout.preferredHeight: implicitHeight
    activeFocusOnTab: true

    Keys.onReturnPressed: root.clicked()
    Keys.onSpacePressed: root.clicked()

    Row {
        id: linkRow
        anchors.verticalCenter: parent.verticalCenter
        spacing: Theme.s8

        Text {
            text: root.label
            color: mouse.containsMouse || root.activeFocus ? Theme.accent : Theme.textMuted
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontBody
            font.underline: mouse.containsMouse
        }

        Text {
            visible: root.suffix !== ""
            text: root.suffix
            color: Theme.accent
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontBody
        }
    }

    Rectangle {
        visible: root.activeFocus
        anchors.left: linkRow.left
        anchors.right: linkRow.right
        anchors.bottom: parent.bottom
        height: 2
        color: Theme.accent
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
