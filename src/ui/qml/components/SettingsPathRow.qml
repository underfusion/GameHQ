import QtQuick
import GameHQ

SettingsRow {
    id: root
    property string path: ""
    property bool showChange: false
    property bool showOpen: true
    property bool showReset: false
    signal changeRequested()
    signal openRequested()
    signal resetRequested()

    icon: "\u25A4"
    description: path
    controlWidth: Theme.s48 * 5

    TextLink {
        visible: root.showReset
        label: "Use default"
        onClicked: root.resetRequested()
    }
    AccentButton {
        visible: root.showChange
        label: "Change"
        quiet: true
        onClicked: root.changeRequested()
    }
    AccentButton {
        visible: root.showOpen
        label: "Open"
        quiet: true
        onClicked: root.openRequested()
    }
}
