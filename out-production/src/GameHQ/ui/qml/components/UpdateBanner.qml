import QtQuick
import QtQuick.Layouts
import GameHQ

// Non-modal notice shown in the desktop gallery window when a newer stable
// release exists (docs/updater.md "Discovery"). This component only ever
// lives in Main.qml — a different window than OverlayWindow.qml — so it can
// never appear over a running game or the pad overlay. Release notes render
// as plain text only (Text.PlainText) — no Markdown/HTML interpretation.
Rectangle {
    id: root

    property bool dismissed: false
    // A newer release (or the user un-skipping via a fresh check) re-arms it.
    onVisibleChanged: if (visible) root.dismissed = false

    visible: updates.latestVersion !== ""
             && ["UpdateAvailable", "Downloading", "ReadyToInstall", "PreparingForUpdate",
                 "Quiescent", "Installing", "Failed"].includes(updates.stateName)
             && !root.dismissed
    height: visible ? implicitHeight : 0
    implicitHeight: content.implicitHeight + Theme.s16 * 2
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

    RowLayout {
        id: content
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.margins: Theme.s16
        spacing: Theme.s16

        ColumnLayout {
            Layout.fillWidth: true
            spacing: Theme.s4

            Text {
                text: {
                    switch (updates.stateName) {
                    case "Downloading":
                        return "Downloading " + Brand.name + " " + updates.latestVersion + "... " + updates.progress + "%"
                    case "ReadyToInstall":
                        return Brand.name + " " + updates.latestVersion + " downloaded and verified"
                    case "PreparingForUpdate":
                    case "Quiescent":
                        return "Getting ready to install " + Brand.name + " " + updates.latestVersion + "..."
                    case "Installing":
                        return "Installing " + Brand.name + " " + updates.latestVersion + "..."
                    default:
                        return Brand.name + " " + updates.latestVersion + " is available"
                    }
                }
                color: Theme.text
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontTitle
                font.weight: Font.DemiBold
            }
            Text {
                text: [Qt.formatDate(updates.publishedAt, "d MMM yyyy"),
                       updates.size > 0 ? root.formattedSize(updates.size) : ""]
                      .filter(s => s !== "").join(" · ")
                color: Theme.textMuted
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontCaption
            }
            Text {
                Layout.fillWidth: true
                visible: text !== ""
                text: updates.stateName === "Failed" ? updates.errorText : updates.notes
                textFormat: Text.PlainText
                color: Theme.textMuted
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontBody
                wrapMode: Text.WordWrap
                maximumLineCount: 3
                elide: Text.ElideRight
            }
            Text {
                Layout.fillWidth: true
                text: "Beta security: SHA-256 detects corruption, but not a compromised GitHub account."
                color: Theme.textMuted
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontCaption
                wrapMode: Text.WordWrap
            }
        }

        AccentButton {
            visible: updates.stateName === "UpdateAvailable"
            label: "Download update (Beta)"
            primary: true
            onClicked: updates.downloadUpdate()
        }
        AccentButton {
            visible: updates.stateName === "Downloading"
            label: "Cancel"
            onClicked: updates.cancelDownload()
        }
        AccentButton {
            visible: updates.stateName === "ReadyToInstall"
            label: "Install and restart"
            primary: true
            onClicked: updates.installAndRestart()
        }
        AccentButton {
            visible: updates.stateName === "Failed" && updates.failedDuringCheck
            label: "Check again"
            primary: true
            onClicked: updates.checkNow()
        }
        AccentButton {
            visible: updates.stateName === "Failed" && !updates.failedDuringCheck
            label: "Retry download"
            primary: true
            onClicked: updates.downloadUpdate()
        }
        AccentButton {
            visible: !["PreparingForUpdate", "Quiescent", "Installing"].includes(updates.stateName)
            label: "View on GitHub"
            onClicked: updates.openReleasePage()
        }
        AccentButton {
            visible: updates.stateName === "UpdateAvailable"
            label: "Skip this version"
            onClicked: updates.skipVersion()
        }
        AccentButton {
            visible: updates.stateName === "UpdateAvailable" || updates.stateName === "Failed"
                     || updates.stateName === "ReadyToInstall"
            label: "Not now"
            onClicked: root.dismissed = true
        }
    }
}
