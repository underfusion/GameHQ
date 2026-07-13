import QtQuick
import QtQuick.Layouts
import GameHQ

RowLayout {
    id: root
    property string label: ""
    property string description: ""
    default property alias controlData: controlHost.data
    Layout.fillWidth: true
    spacing: Theme.s16

    ColumnLayout {
        Layout.fillWidth: true
        spacing: Theme.s4
        Text {
            text: root.label
            color: Theme.text
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontBody
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
        Text {
            visible: root.description.length > 0
            text: root.description
            color: Theme.textMuted
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontCaption
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
    }
    RowLayout { id: controlHost; spacing: Theme.s8 }
}
