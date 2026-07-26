import QtQuick
import QtQuick.Dialogs
import GameHQ
import "../components"

SettingsPage {
    id: root
    pageTitle: "Library"
    pageDescription: "Review every folder GameHQ manages or scans for media."
    pageAction: Component {
        AccentButton {
            label: "Rescan now"
            quiet: true
            onClicked: {
                app.rescan()
                sounds.play("confirm")
            }
        }
    }

    function openFolder(path) {
        Qt.openUrlExternally("file:///" + path.replace(/\\/g, "/"))
    }

    SettingsSection {
        eyebrow: "Storage"
        title: "Managed locations"
        badge: "2 active"
        description: "Current output folders and earlier roots remain scanned so past media stays visible."

        SettingsPathRow {
            label: "Screenshots"
            path: app.screenshotsRoot
            onOpenRequested: root.openFolder(app.screenshotsRoot)
        }
        SettingsPathRow {
            label: "Replay clips"
            path: app.clipsRoot
            onOpenRequested: root.openFolder(app.clipsRoot)
        }
        Repeater {
            model: app.managedRoots.filter(function(path) {
                return path !== app.screenshotsRoot && path !== app.clipsRoot
                       && path !== app.capturesRoot
            })
            delegate: SettingsPathRow {
                required property string modelData
                label: "Previous location"
                path: modelData
                showDivider: index < app.managedRoots.length - 1
                onOpenRequested: root.openFolder(modelData)
            }
        }
    }

    SettingsSection {
        eyebrow: "Imports"
        title: "Watched folders"
        badge: app.watchedFolders.length + (app.watchedFolders.length === 1 ? " folder" : " folders")
        description: "External folders are scanned read-only and never become GameHQ output locations."

        SettingsEmptyState {
            visible: app.watchedFolders.length === 0
            icon: "\uFF0B"
            title: "No watched folders yet"
            description: "Add folders created by Steam, OBS, Xbox Game Bar, or another capture tool."
            AccentButton {
                label: "Add watched folder"
                primary: true
                onClicked: watchedFolderDialog.open()
            }
        }

        Repeater {
            model: app.watchedFolders
            delegate: SettingsRow {
                required property string modelData
                icon: "\u25A4"
                label: "Watched folder"
                description: modelData
                controlWidth: Theme.s48 * 5
                AccentButton {
                    label: "Open"
                    quiet: true
                    onClicked: root.openFolder(modelData)
                }
                AccentButton {
                    label: "Remove"
                    quiet: true
                    labelColor: Theme.danger
                    borderColor: Theme.danger
                    onClicked: app.removeWatchedFolder(modelData)
                }
            }
        }

        AccentButton {
            visible: app.watchedFolders.length > 0
            label: "Add watched folder"
            quiet: true
            onClicked: watchedFolderDialog.open()
        }
    }

    SettingsSection {
        eyebrow: "Last scan"
        title: app.lastScanAvailable
               ? (app.lastScanAdded === 0 ? "Library is up to date"
                                          : app.lastScanAdded + " new capture(s) added")
               : "Not scanned this session"
        variant: "compact"
        description: "Rescan checks current, previous, and watched locations for media missing from the library."
    }

    FolderDialog {
        id: watchedFolderDialog
        title: "Choose a folder to watch"
        onAccepted: app.addWatchedFolder(selectedFolder)
    }
}
