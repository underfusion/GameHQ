import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic as QC
import GameHQ

// One clearly labeled assignment slot. Edit/Add is the dominant interaction;
// Remove stays inside the same card so it cannot be mistaken for another slot.
Rectangle {
    id: root

    property string slotLabel: ""
    property string triggerLabel: ""
    property string badgeLabel: ""
    property bool assigned: false
    property bool editable: true
    signal editRequested()
    signal clearRequested()

    implicitHeight: Theme.s48 + Theme.s24
    radius: Theme.radiusM
    color: root.assigned ? Theme.surface : "transparent"
    border.width: Theme.borderWidth
    border.color: root.assigned ? Theme.borderLight : Theme.stroke

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.s8
        spacing: Theme.s4

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: Theme.s24
            spacing: Theme.s8

            Text {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.s8
                text: root.slotLabel.toUpperCase() + " ASSIGNMENT"
                color: Theme.textFaint
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontCaption
                font.letterSpacing: Theme.letterSpacingWide
                elide: Text.ElideRight
            }

            QC.AbstractButton {
                id: clearButton

                visible: root.assigned && root.editable
                Layout.preferredWidth: removeText.implicitWidth + Theme.s16
                Layout.preferredHeight: Theme.s24
                focusPolicy: Qt.StrongFocus
                Accessible.name: "Remove " + root.slotLabel.toLowerCase() + " assignment"
                Accessible.role: Accessible.Button
                onClicked: root.clearRequested()

                contentItem: Text {
                    id: removeText
                    text: "Remove"
                    color: clearButton.activeFocus || clearButton.hovered
                           ? Theme.danger : Theme.textMuted
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontCaption
                    font.weight: Font.DemiBold
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: Theme.radiusS
                    color: clearButton.hovered || clearButton.down ? Theme.dangerSoft : "transparent"
                    border.width: clearButton.activeFocus ? Theme.borderWidth + 1 : 0
                    border.color: Theme.focusRing
                    Behavior on color { ColorAnimation { duration: Theme.durFast } }
                }
            }
        }

        QC.AbstractButton {
            id: valueField

            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumWidth: 0
            enabled: root.editable
            focusPolicy: root.editable ? Qt.StrongFocus : Qt.NoFocus
            leftPadding: Theme.s8
            rightPadding: Theme.s8
            Accessible.name: root.assigned
                             ? "Edit " + root.slotLabel.toLowerCase() + " assignment: "
                               + root.triggerLabel + ", " + root.badgeLabel
                             : "Add " + root.slotLabel.toLowerCase() + " assignment"
            Accessible.role: Accessible.Button
            onClicked: root.editRequested()

            contentItem: RowLayout {
                spacing: Theme.s8

                Text {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    text: root.assigned ? root.triggerLabel : "+ Add input"
                    color: root.assigned ? Theme.text : Theme.textMuted
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontBody
                    font.weight: root.assigned ? Font.DemiBold : Font.Normal
                    elide: Text.ElideRight
                    verticalAlignment: Text.AlignVCenter
                }

                Rectangle {
                    visible: root.assigned && root.badgeLabel !== ""
                    implicitWidth: badgeText.implicitWidth + Theme.s12
                    implicitHeight: badgeText.implicitHeight + Theme.s4
                    radius: Theme.radiusPill
                    color: Theme.bg1
                    border.width: Theme.borderWidth
                    border.color: Theme.stroke

                    Text {
                        id: badgeText
                        anchors.centerIn: parent
                        text: root.badgeLabel
                        color: Theme.textMuted
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontCaption
                    }
                }

                Text {
                    text: root.editable ? (root.assigned ? "EDIT  \u203a" : "ADD  \u203a") : "FIXED"
                    color: valueField.activeFocus || valueField.hovered
                           ? Theme.accent : Theme.textMuted
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontCaption
                    font.weight: Font.DemiBold
                    font.letterSpacing: Theme.letterSpacingWide
                }
            }

            background: Rectangle {
                radius: Theme.radiusS
                color: valueField.down || valueField.hovered ? Theme.surfaceHover : "transparent"
                border.width: valueField.activeFocus ? Theme.borderWidth + 1 : 0
                border.color: Theme.focusRing
                Behavior on color { ColorAnimation { duration: Theme.durFast } }
            }
        }
    }
}
