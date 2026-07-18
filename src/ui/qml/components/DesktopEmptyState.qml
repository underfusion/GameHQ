import QtQuick
import GameHQ

Column {
    id: root

    signal addFolderRequested()

    anchors.centerIn: parent
    spacing: Theme.s12

    Text {
        text: "\u25a6"
        color: Theme.textFaint
        font.pixelSize: Theme.fontHero
        anchors.horizontalCenter: parent.horizontalCenter
    }

    Text {
        text: "No captures yet \u2014 add a folder to watch."
        color: Theme.textMuted
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontBody
    }

    AccentButton {
        anchors.horizontalCenter: parent.horizontalCenter
        primary: true
        label: "Add folder\u2026"
        onClicked: root.addFolderRequested()
    }
}
