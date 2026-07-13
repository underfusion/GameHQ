import QtQuick
import GameHQ

// Reusable modal confirmation dialog (scrim + centred card + Cancel/confirm).
// Styled from Theme. open()/close(); emits confirmed() or canceled().
Item {
    id: root

    property string title: "Are you sure?"
    property string message: ""
    property string confirmLabel: "Delete"
    signal confirmed()
    signal canceled()

    visible: false
    opacity: 0

    function open()  { visible = true }
    function close() { visible = false }

    Behavior on opacity { NumberAnimation { duration: Theme.durFast } }
    states: State {
        when: root.visible
        PropertyChanges { target: root; opacity: 1 }
    }

    // Scrim — clicking outside the card cancels.
    Rectangle {
        anchors.fill: parent
        color: Theme.scrim
        MouseArea {
            anchors.fill: parent
            onClicked: { root.canceled(); root.close() }
        }
    }

    Rectangle {
        id: dialog
        anchors.centerIn: parent
        width: 400
        height: col.implicitHeight + Theme.s24 * 2
        radius: Theme.radiusL
        color: Theme.surface
        border.width: 1
        border.color: Theme.stroke

        MouseArea { anchors.fill: parent }   // swallow clicks so the scrim doesn't cancel

        Column {
            id: col
            x: Theme.s24
            y: Theme.s24
            width: parent.width - Theme.s24 * 2
            spacing: Theme.s16

            Text {
                width: parent.width
                text: root.title
                color: Theme.text
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontTitle
                font.weight: Font.DemiBold
                wrapMode: Text.WordWrap
            }
            Text {
                width: parent.width
                visible: root.message !== ""
                text: root.message
                color: Theme.textMuted
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontBody
                wrapMode: Text.WordWrap
            }
            Row {
                anchors.right: parent.right
                spacing: Theme.s12
                AccentButton {
                    label: "Cancel"
                    onClicked: { root.canceled(); root.close() }
                }
                AccentButton {
                    primary: true
                    label: root.confirmLabel
                    onClicked: { root.confirmed(); root.close() }
                }
            }
        }
    }
}
