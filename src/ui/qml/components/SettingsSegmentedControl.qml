import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic as QC
import GameHQ

Rectangle {
    id: root
    property var options: []
    property string currentValue: ""
    signal activated(string value)

    implicitWidth: segmentRow.implicitWidth + Theme.s4
    implicitHeight: 38
    radius: Theme.radiusS
    color: Theme.bg1
    border.width: Theme.borderWidth
    border.color: Theme.stroke

    RowLayout {
        id: segmentRow
        anchors.fill: parent
        anchors.margins: 2
        spacing: 2

        Repeater {
            model: root.options
            delegate: QC.AbstractButton {
                id: segment
                required property var modelData
                property bool selected: root.currentValue === modelData.value
                implicitWidth: labelText.implicitWidth + Theme.s24
                implicitHeight: 32
                focusPolicy: Qt.StrongFocus
                onClicked: root.activated(modelData.value)
                contentItem: Text {
                    id: labelText
                    text: segment.modelData.label
                    color: segment.selected ? Theme.textOnAccent : Theme.textMuted
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontCaption
                    font.weight: segment.selected ? Font.DemiBold : Font.Normal
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: Math.max(0, Theme.radiusS - 2)
                    color: segment.selected ? Theme.accent
                                            : (segment.hovered || segment.activeFocus ? Theme.surfaceHover : "transparent")
                    border.width: segment.activeFocus ? 1 : 0
                    border.color: Theme.focusRing
                }
            }
        }
    }
}
