import QtQuick
import QtQuick.Layouts
import GameHQ

ColumnLayout {
    id: root
    property string title: ""
    property string description: ""
    default property alias contentData: contentColumn.data
    Layout.fillWidth: true
    spacing: Theme.s8

    Text {
        text: root.title.toUpperCase()
        color: Theme.textFaint
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontCaption
        font.letterSpacing: Theme.letterSpacingWide
    }
    Text {
        visible: root.description.length > 0
        text: root.description
        color: Theme.textMuted
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontBody
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
    }
    Rectangle {
        Layout.fillWidth: true
        implicitHeight: contentColumn.implicitHeight + Theme.s16 * 2
        radius: Theme.radiusM
        color: Theme.surface
        border.width: 1
        border.color: Theme.stroke
        ColumnLayout {
            id: contentColumn
            anchors.fill: parent
            anchors.margins: Theme.s16
            spacing: Theme.s12
        }
    }
}
