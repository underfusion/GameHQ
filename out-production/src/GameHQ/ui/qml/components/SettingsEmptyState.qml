import QtQuick
import QtQuick.Layouts
import GameHQ

ColumnLayout {
    id: root
    property string icon: "\uFF0B"
    property string title: ""
    property string description: ""
    default property alias actionData: actionHost.data
    Layout.fillWidth: true
    Layout.minimumWidth: 0
    spacing: Theme.s8

    Text {
        text: root.icon
        color: Theme.textFaint
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontTitle
        Layout.alignment: Qt.AlignHCenter
    }
    Text {
        text: root.title
        color: Theme.text
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontBody
        font.weight: Font.DemiBold
        Layout.alignment: Qt.AlignHCenter
    }
    Text {
        text: root.description
        color: Theme.textMuted
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontCaption
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
        Layout.minimumWidth: 0
    }
    RowLayout { id: actionHost; Layout.alignment: Qt.AlignHCenter; spacing: Theme.s8 }
}
