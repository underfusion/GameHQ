import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import GameHQ

RowLayout {
    id: root

    property string titleText: ""
    property bool bulkMode: false
    property int bulkCount: 0
    property bool bulkAllSelected: false

    signal bulkEnterRequested()
    signal bulkSelectAllRequested()
    signal bulkDeleteRequested()
    signal bulkExitRequested()

    Layout.fillWidth: true

    Text {
        text: root.bulkMode ? (root.bulkCount + " selected") : root.titleText
        color: Theme.text
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontDisplay
        font.weight: Font.Light
        Layout.fillWidth: true
    }

    AccentButton {
        visible: !root.bulkMode
        quiet: true
        icon: "\u2611"
        label: "Bulk Select"
        onClicked: root.bulkEnterRequested()
    }

    RowLayout {
        visible: root.bulkMode
        spacing: Theme.s8

        AccentButton {
            quiet: true
            icon: root.bulkAllSelected ? "\u2610" : "\u2611"
            label: root.bulkAllSelected ? "Deselect all" : "Select all"
            onClicked: root.bulkSelectAllRequested()
        }

        AccentButton {
            quiet: true
            icon: "\uE74D"
            iconFontFamily: "Segoe Fluent Icons"
            iconColor: Theme.danger
            labelColor: Theme.danger
            borderColor: Theme.danger
            quietIdleBorderColor: Theme.dangerQuietBorder
            quietTopColor: Theme.dangerQuietTop
            quietBottomColor: Theme.dangerQuietBottom
            label: "Delete"
            opacity: root.bulkCount === 0 ? 0.45 : 1.0
            enabled: root.bulkCount > 0
            onClicked: root.bulkDeleteRequested()
        }

        AccentButton {
            quiet: true
            icon: "\u2713"
            borderColor: Theme.success
            quietIdleBorderColor: Theme.successQuietBorder
            quietTopColor: Theme.successQuietTop
            quietBottomColor: Theme.successQuietBottom
            label: "Done"
            onClicked: root.bulkExitRequested()
        }
    }
}
