import QtQuick
import QtQuick.Layouts
import GameHQ

RowLayout {
    id: root
    property string title: ""
    property string description: ""
    property Component action

    spacing: Theme.s16
    Layout.minimumWidth: 0

    ColumnLayout {
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        spacing: Theme.s4
        Text {
            text: root.title
            color: Theme.text
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontTitle
            font.weight: Font.DemiBold
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
            Layout.minimumWidth: 0
        }
        Text {
            visible: root.description.length > 0
            text: root.description
            color: Theme.textMuted
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontBody
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
            Layout.minimumWidth: 0
        }
    }

    Loader {
        visible: status === Loader.Ready
        sourceComponent: root.action
        Layout.alignment: Qt.AlignTop | Qt.AlignRight
    }
}
