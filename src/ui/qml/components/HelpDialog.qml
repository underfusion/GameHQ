import QtQuick
import QtQuick.Layouts
import GameHQ

Item {
    id: root

    signal closed()

    visible: false
    opacity: 0
    focus: visible

    function open() {
        visible = true
        helpView.resetScroll()
        Qt.callLater(closeButton.forceActiveFocus)
    }

    function close() {
        if (!visible)
            return
        visible = false
        closed()
    }

    function padScroll(direction) {
        helpView.scrollBy(direction)
        sounds.play("nav_tick")
    }

    function padConfirm() {
        closeButton.clicked()
    }

    Behavior on opacity { NumberAnimation { duration: Theme.durFast } }
    states: State {
        when: root.visible
        PropertyChanges { target: root; opacity: 1 }
    }

    Keys.priority: Keys.BeforeItem
    Keys.onEscapePressed: function(event) {
        event.accepted = true
        root.close()
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.scrim
        MouseArea {
            anchors.fill: parent
            onClicked: root.close()
            onWheel: wheel => wheel.accepted = true
        }
    }

    Rectangle {
        id: card
        anchors.centerIn: parent
        width: Math.min(760, root.width - Theme.s48)
        height: Math.min(660, root.height - Theme.s48)
        radius: Theme.radiusL
        color: Theme.surface
        border.width: 1
        border.color: Theme.stroke
        clip: true

        MouseArea {
            anchors.fill: parent
            onWheel: wheel => wheel.accepted = true
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: Theme.s24
            spacing: Theme.s16

            RowLayout {
                Layout.fillWidth: true

                Text {
                    Layout.fillWidth: true
                    text: "Help"
                    color: Theme.text
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontDisplay
                    font.weight: Font.Light
                }

                DialogCloseButton {
                    Layout.alignment: Qt.AlignTop
                    onClicked: root.close()
                }
            }

            HelpView {
                id: helpView
                Layout.fillWidth: true
                Layout.fillHeight: true
            }

            AccentButton {
                id: closeButton
                label: "Close"
                icon: "×"
                borderColor: Theme.borderLight
                labelColor: Theme.textMuted
                iconColor: Theme.textMuted
                Layout.alignment: Qt.AlignRight
                onClicked: root.close()
            }
        }
    }
}
