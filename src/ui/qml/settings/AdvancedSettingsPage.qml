import QtQuick
import QtQuick.Layouts
import GameHQ
import "../components"

SettingsPage {
    signal restoreAllRequested()
    signal restoreCategoryRequested(string category)
    signal restoreInputRequested()
    SettingsSection {
        title: "Application"
        SettingsRow {
            label: "Version"
            Text { text: app.version; color: Theme.textMuted; font.family: Theme.fontFamily; font.pixelSize: Theme.fontBody }
        }
        SettingsRow {
            label: "Storage mode"
            Text {
                text: app.portableMode ? "Portable" : "Installed"
                color: Theme.textMuted
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontBody
            }
        }
    }
    SettingsSection {
        title: "Diagnostics"
        description: "Open " + Brand.name + "-owned folders for logs, configuration, database, and cached support data."
        SettingsRow {
            label: "Logs folder"
            description: app.logsRoot
            AccentButton { label: "Open"; primary: true; onClicked: app.openLogsFolder() }
        }
        SettingsRow {
            label: "Data folder"
            description: app.dataRoot
            AccentButton { label: "Open"; primary: true; onClicked: app.openDataFolder() }
        }
        SettingsRow {
            label: "Diagnostic summary"
            description: "Copies version, storage mode, and every managed folder path — useful when reporting a problem."
            AccentButton {
                label: "Copy to clipboard"
                primary: true
                onClicked: { app.copyDiagnosticSummary(); sounds.play("confirm") }
            }
        }
    }
    SettingsSection {
        title: "Restore"
        description: "Restoring settings never deletes captures, favorites, watched media, or database records."
        SettingsRow {
            label: "Restore a category"
            description: "Only that category's options return to defaults; everything else is untouched."
            RowLayout {
                spacing: Theme.s8
                AccentButton { label: "General"; primary: true; onClicked: restoreCategoryRequested("General") }
                AccentButton { label: "Capture"; primary: true; onClicked: restoreCategoryRequested("Capture") }
                AccentButton { label: "Replay"; primary: true; onClicked: restoreCategoryRequested("Replay") }
                AccentButton { label: "Notifications & Sound"; primary: true; onClicked: restoreCategoryRequested("Notifications & Sound") }
            }
        }
        SettingsRow {
            label: "Restore all input bindings"
            description: "Controller, keyboard, and mouse overrides return to their built-in defaults."
            AccentButton { label: "Restore"; primary: true; onClicked: restoreInputRequested() }
        }
        AccentButton { label: "Restore all settings"; primary: true; onClicked: restoreAllRequested() }
    }
}
