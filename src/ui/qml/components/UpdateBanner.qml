import QtQuick
import QtQuick.Layouts
import GameHQ

// Compact desktop-only notice. AboutWhatsNewDialog owns release summaries,
// full notes, and update choices so the app never maintains two changelog UIs.
Rectangle {
    id: root

    property bool dismissed: false
    signal detailsRequested()

    onVisibleChanged: if (visible) root.dismissed = false

    visible: updates.latestVersion !== ""
             && ["UpdateAvailable", "Downloading", "ReadyToInstall", "PreparingForUpdate",
                 "Quiescent", "Installing", "Failed"].includes(updates.stateName)
             && !root.dismissed
    height: visible ? implicitHeight : 0
    implicitHeight: content.implicitHeight + Theme.s12 * 2
    color: Theme.surface
    radius: Theme.radiusM
    border.width: 1
    border.color: Theme.accent
    clip: true

    Behavior on height {
        NumberAnimation { duration: Theme.durNormal; easing.type: Easing.OutCubic }
    }

    function formattedSize(bytes) {
        if (bytes >= 1024 * 1024)
            return (bytes / (1024 * 1024)).toFixed(1) + " MB"
        if (bytes >= 1024)
            return Math.round(bytes / 1024) + " KB"
        return bytes + " B"
    }

    function titleText() {
        switch (updates.stateName) {
        case "Downloading": return "Downloading " + Brand.name + " " + updates.latestVersion
        case "ReadyToInstall": return Brand.name + " " + updates.latestVersion + " is ready"
        case "PreparingForUpdate":
        case "Quiescent": return "Preparing " + Brand.name + " " + updates.latestVersion
        case "Installing": return "Installing " + Brand.name + " " + updates.latestVersion
        case "Failed": return "The update needs attention"
        default: return Brand.name + " " + updates.latestVersion + " is available"
        }
    }

    function detailText() {
        switch (updates.stateName) {
        case "Downloading": return updates.progress + "% complete"
        case "ReadyToInstall": return "Download verified and ready to install."
        case "PreparingForUpdate":
        case "Quiescent": return "Waiting for capture work to finish safely."
        case "Installing": return "GameHQ will restart when installation finishes."
        case "Failed": return updates.errorText !== "" ? updates.errorText
                                                         : "Open the update details to continue."
        default:
            return [Qt.formatDate(updates.publishedAt, "d MMM yyyy"),
                    updates.size > 0 ? root.formattedSize(updates.size) : ""]
                   .filter(value => value !== "").join(" \u00b7 ")
        }
    }

    RowLayout {
        id: content
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.margins: Theme.s12
        spacing: Theme.s12

        Rectangle {
            Layout.preferredWidth: 40
            Layout.preferredHeight: 40
            radius: Theme.radiusM
            color: Theme.hoverTint

            Image {
                anchors.centerIn: parent
                width: 26
                height: 26
                source: "qrc:/icons/gamehq.svg"
                sourceSize.width: 26
                sourceSize.height: 26
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: Theme.s4

            Text {
                Layout.fillWidth: true
                text: root.titleText()
                textFormat: Text.PlainText
                color: Theme.text
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontH3
                font.weight: Font.DemiBold
                elide: Text.ElideRight
            }
            Text {
                Layout.fillWidth: true
                text: root.detailText()
                textFormat: Text.PlainText
                color: updates.stateName === "Failed" ? Theme.danger : Theme.textMuted
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontCaption
                elide: Text.ElideRight
            }
        }

        AccentButton {
            visible: ["UpdateAvailable", "Downloading", "ReadyToInstall", "Failed"]
                     .includes(updates.stateName)
            label: updates.stateName === "UpdateAvailable" ? "See what's new" : "View update"
            primary: updates.stateName === "UpdateAvailable"
            onClicked: root.detailsRequested()
        }
        AccentButton {
            visible: updates.stateName === "Downloading" || updates.stateName === "ReadyToInstall"
            label: updates.stateName === "Downloading" ? "Cancel" : "Install and restart"
            primary: updates.stateName === "ReadyToInstall"
            onClicked: updates.stateName === "Downloading"
                       ? updates.cancelDownload() : updates.installAndRestart()
        }
        AccentButton {
            visible: updates.stateName === "UpdateAvailable" || updates.stateName === "Failed"
                     || updates.stateName === "ReadyToInstall"
            label: "Not now"
            onClicked: root.dismissed = true
        }
    }
}
