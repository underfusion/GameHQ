import QtQuick
import QtQuick.Layouts
import GameHQ

GridLayout {
    id: root
    property var items: []
    columns: width < 720 ? 2 : 4
    columnSpacing: Theme.s8
    rowSpacing: Theme.s8
    Layout.fillWidth: true

    Repeater {
        model: root.items
        delegate: Rectangle {
            id: statusTile
            required property var modelData
            Layout.fillWidth: true
            implicitHeight: statusColumn.implicitHeight + Theme.s16 * 2
            radius: Theme.radiusS
            color: Theme.bg1
            border.width: Theme.borderWidth
            border.color: Theme.stroke
            ColumnLayout {
                id: statusColumn
                anchors.fill: parent
                anchors.margins: Theme.s16
                spacing: Theme.s4
                Text {
                    text: statusTile.modelData.label.toUpperCase()
                    color: Theme.textFaint
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontCaption
                    font.letterSpacing: Theme.letterSpacingWide
                }
                Text {
                    text: "\u25CF  " + statusTile.modelData.value
                    color: statusTile.modelData.tone === "warning" ? Theme.warning
                         : statusTile.modelData.tone === "danger" ? Theme.danger
                         : statusTile.modelData.tone === "accent" ? Theme.accent
                                                                  : Theme.success
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontBody
                    font.weight: Font.DemiBold
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
                Text {
                    visible: statusTile.modelData.detail !== undefined
                             && statusTile.modelData.detail.length > 0
                    text: statusTile.modelData.detail || ""
                    color: Theme.textMuted
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontCaption
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
            }
        }
    }
}
