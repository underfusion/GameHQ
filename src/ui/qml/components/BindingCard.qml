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
    property string changeState: "default"
    property string statusLabel: ""
    readonly property bool changed: changeState !== "default"
    signal editRequested()
    signal clearRequested()
    signal resetRequested()

    implicitHeight: Theme.s48 + Theme.s24
    radius: Theme.radiusM
    color: root.changed ? Theme.accentSoft : root.assigned ? Theme.surface : "transparent"
    border.width: Theme.borderWidth
    border.color: root.changed ? Theme.accent : root.assigned ? Theme.borderLight : Theme.stroke

    // Half-height and vertically centred so the accent bar never pokes past
    // the card's rounded left corners.
    Rectangle {
        visible: root.changed
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        width: 3
        height: parent.height / 2
        radius: width / 2
        color: Theme.accent
    }

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

            Rectangle {
                visible: root.statusLabel !== ""
                implicitWidth: statusText.implicitWidth + Theme.s12
                implicitHeight: statusText.implicitHeight + Theme.s4
                radius: Theme.radiusPill
                color: Theme.accentSoft
                border.width: Theme.borderWidth
                border.color: Theme.accent

                Text {
                    id: statusText
                    anchors.centerIn: parent
                    text: root.statusLabel
                    color: Theme.accent
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontCaption
                    font.weight: Font.DemiBold
                }
            }

            QC.AbstractButton {
                id: resetButton

                visible: root.editable
                         && (root.changeState === "modified" || root.changeState === "removed")
                Layout.preferredWidth: resetText.implicitWidth + Theme.s16
                Layout.preferredHeight: Theme.s24
                focusPolicy: Qt.StrongFocus
                Accessible.name: (root.changeState === "removed" ? "Restore " : "Revert ")
                                 + root.slotLabel.toLowerCase() + " assignment"
                Accessible.role: Accessible.Button
                onClicked: root.resetRequested()

                contentItem: Text {
                    id: resetText
                    text: root.changeState === "removed" ? "Restore" : "Revert"
                    color: resetButton.activeFocus || resetButton.hovered
                           ? Theme.accent : Theme.textMuted
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontCaption
                    font.weight: Font.DemiBold
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: Theme.radiusS
                    color: resetButton.hovered || resetButton.down ? Theme.accentSoft : "transparent"
                    border.width: resetButton.activeFocus ? Theme.borderWidth + 1 : 0
                    border.color: Theme.focusRing
                    Behavior on color { ColorAnimation { duration: Theme.durFast } }
                }
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
            Accessible.name: (root.assigned
                              ? "Edit " + root.slotLabel.toLowerCase() + " assignment: "
                                + root.triggerLabel + ", " + root.badgeLabel
                              : "Add " + root.slotLabel.toLowerCase() + " assignment")
                             + (root.statusLabel !== "" ? ", status " + root.statusLabel : "")
            Accessible.role: Accessible.Button
            onClicked: root.editRequested()

            contentItem: RowLayout {
                spacing: Theme.s8

                Text {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    text: root.assigned ? root.triggerLabel
                                        : root.changeState === "removed" ? "Unassigned" : "+ Add input"
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
