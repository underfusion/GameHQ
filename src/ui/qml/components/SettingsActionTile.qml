import QtQuick
import QtQuick.Layouts
import GameHQ

Rectangle {
    id: root
    property string icon: ""
    property string title: ""
    property string description: ""
    property string tone: "normal"
    signal clicked()

    Layout.fillWidth: true
    implicitHeight: tileRow.implicitHeight + Theme.s16 * 2
    radius: Theme.radiusS
    color: mouse.containsMouse || activeFocus ? Theme.surfaceElevated : Theme.bg1
    border.width: activeFocus ? 2 : Theme.borderWidth
    border.color: activeFocus ? Theme.focusRing : Theme.stroke
    activeFocusOnTab: true
    Keys.onReturnPressed: clicked()
    Keys.onSpacePressed: clicked()

    RowLayout {
        id: tileRow
        anchors.fill: parent
        anchors.margins: Theme.s16
        spacing: Theme.s12
        Text {
            text: root.icon
            color: root.tone === "warning" ? Theme.warning
                 : root.tone === "danger" ? Theme.danger : Theme.accent
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontH3
            Layout.preferredWidth: Theme.s24
            horizontalAlignment: Text.AlignHCenter
        }
        ColumnLayout {
            Layout.fillWidth: true
            spacing: Theme.s4
            Text {
                text: root.title
                color: Theme.text
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontBody
                font.weight: Font.DemiBold
                Layout.fillWidth: true
            }
            Text {
                text: root.description
                color: Theme.textMuted
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontCaption
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
        }
    }
    MouseArea { id: mouse; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: root.clicked() }
}
