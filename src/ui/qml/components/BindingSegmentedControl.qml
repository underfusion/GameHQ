import QtQuick
import QtQuick.Controls.Basic as QC
import GameHQ

// A compact segmented selector that can wrap on narrow dialog widths.
Rectangle {
    id: root

    property var options: []
    property string currentValue: ""
    signal activated(string value)

    implicitWidth: Theme.dialogWidth
    implicitHeight: Math.max(Theme.s32 + Theme.s4,
                             segmentFlow.childrenRect.height + Theme.s4)
    radius: Theme.radiusS
    color: Theme.bg1
    border.width: Theme.borderWidth
    border.color: Theme.stroke

    Flow {
        id: segmentFlow
        x: Theme.s4 / 2
        y: Theme.s4 / 2
        width: parent.width - Theme.s4
        spacing: Theme.s4 / 2

        Repeater {
            model: root.options
            delegate: QC.AbstractButton {
                id: segment
                required property var modelData
                readonly property bool selected: root.currentValue === modelData.value
                implicitWidth: segmentLabel.implicitWidth + Theme.s24
                implicitHeight: Theme.s32
                focusPolicy: Qt.StrongFocus
                Accessible.name: modelData.label
                Accessible.role: Accessible.RadioButton
                Accessible.checked: selected
                onClicked: root.activated(modelData.value)

                contentItem: Text {
                    id: segmentLabel
                    text: segment.modelData.label
                    color: segment.selected ? Theme.textOnAccent : Theme.textMuted
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontCaption
                    font.weight: segment.selected ? Font.DemiBold : Font.Normal
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: Math.max(0, Theme.radiusS - Theme.borderWidth)
                    color: segment.selected ? Theme.accent
                                            : segment.hovered || segment.activeFocus
                                              ? Theme.surfaceHover : "transparent"
                    border.width: segment.activeFocus ? Theme.borderWidth : 0
                    border.color: Theme.focusRing
                }
            }
        }
    }
}
