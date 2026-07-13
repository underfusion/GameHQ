import QtQuick
import QtQuick.Dialogs
import QtQuick.Layouts
import GameHQ
import "../components"

SettingsPage {
    SettingsSection {
        title: "Library maintenance"
        description: "Scan every current, previous, and watched root for media that is not yet in the library."
        SettingsRow {
            label: "Library scan"
            description: app.lastScanAvailable
                ? (app.lastScanAdded === 0 ? "Last scan found nothing new." : "Last scan added " + app.lastScanAdded + " capture(s).")
                : "Not scanned yet this session."
            AccentButton {
                label: "Rescan now"
                primary: true
                onClicked: {
                    app.rescan()
                    sounds.play("confirm")
                }
            }
        }
    }

    SettingsSection {
        title: "Managed locations"
        description: "Folders " + Brand.name + " writes new captures to, plus any earlier locations still scanned so past media stays visible."
        SettingsRow {
            label: "Screenshots"
            description: app.screenshotsRoot
        }
        SettingsRow {
            label: "Clips"
            description: app.clipsRoot
        }
        Text {
            property var previousRoots: app.managedRoots.filter(
                r => r !== app.screenshotsRoot && r !== app.clipsRoot && r !== app.capturesRoot)
            visible: previousRoots.length > 0
            text: "Also scanned (previously used): " + previousRoots.join(", ")
            color: Theme.textMuted
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontCaption
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
    }

    SettingsSection {
        title: "Watched folders"
        description: "Imported folders are scanned read-only and never become " + Brand.name + "-managed output locations."
        Text {
            visible: app.watchedFolders.length === 0
            text: "No watched folders yet."
            color: Theme.textMuted
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontBody
        }
        Repeater {
            model: app.watchedFolders
            delegate: SettingsRow {
                label: modelData
                AccentButton { label: "Remove"; primary: true; onClicked: app.removeWatchedFolder(modelData) }
            }
        }
        AccentButton { label: "Add folder..."; primary: true; onClicked: watchedFolderDialog.open() }
    }
    FolderDialog {
        id: watchedFolderDialog
        title: "Choose a folder to watch"
        onAccepted: app.addWatchedFolder(selectedFolder)
    }
}
