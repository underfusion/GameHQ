import QtQuick
import QtQuick.Dialogs
import QtQuick.Layouts
import GameHQ
import "../components"

SettingsPage {
    id: root
    pageTitle: "Advanced"
    pageDescription: "Review system health, open diagnostic resources, and recover settings."

    signal restoreAllRequested()
    signal restoreCategoryRequested(string category)
    signal restoreInputRequested()
    signal importPortableRequested(url folderUrl)

    function hdrDetailItems() {
        return String(app.hdrDetailText || "").split(/\r?\n/)
            .filter(function(line) { return line.trim().length > 0 })
            .map(function(line) {
                const separator = line.indexOf(":")
                return separator > 0
                    ? { label: line.slice(0, separator).trim(),
                        value: line.slice(separator + 1).trim() }
                    : { label: "", value: line.trim() }
            })
    }

    SettingsSection {
        eyebrow: "Overview"
        title: "System status"
        description: "A concise view of the environment GameHQ is currently using."
        SettingsStatusStrip {
            items: [
                { label: "System", value: "Ready", detail: app.portableMode ? "Portable profile" : "Installed profile" },
                { label: "Capture",
                  value: app.hdrDisplayActive ? "HDR Active" : "HDR Inactive",
                  detail: app.hdrStatusText,
                  tone: app.hdrStatusText.toLowerCase().indexOf("unavailable") >= 0
                        ? "warning" : app.hdrDisplayActive ? "accent" : "danger" },
                { label: "Storage", value: "Healthy", detail: "Managed folders available" },
                { label: "Version", value: app.version, detail: "Current installation" }
            ]
        }
    }

    SettingsSection {
        eyebrow: "Resources"
        title: "Locations"
        description: "Open GameHQ-owned folders used for logs, configuration, database, and support data."
        SettingsPathRow {
            label: "Logs folder"
            path: app.logsRoot
            onOpenRequested: app.openLogsFolder()
        }
        SettingsPathRow {
            label: "Data folder"
            path: app.dataRoot
            showDivider: false
            onOpenRequested: app.openDataFolder()
        }
    }

    SettingsSection {
        eyebrow: "Diagnostics"
        title: "Tools"
        description: "Collect support information or refresh hardware status without changing settings."
        GridLayout {
            Layout.fillWidth: true
            columns: width < 720 ? 1 : 2
            columnSpacing: Theme.s8
            rowSpacing: Theme.s8
            SettingsActionTile {
                icon: "\u2398"
                title: "Copy diagnostic summary"
                description: "Version, profile mode, and managed paths."
                onClicked: { app.copyDiagnosticSummary(); sounds.play("confirm") }
            }
            SettingsActionTile {
                icon: "\u21BB"
                title: "Refresh display status"
                description: "Recheck HDR and capture capabilities."
                onClicked: app.refreshHdrStatus()
            }
            SettingsActionTile {
                visible: !app.portableMode
                icon: "\u21E5"
                title: "Import portable profile"
                description: "Validate, stage, and import a fresh portable profile."
                onClicked: portableFolderDialog.open()
            }
        }
    }

    SettingsSection {
        eyebrow: "Display capture"
        title: "HDR details"
        status: app.hdrStatusText
        statusColor: app.hdrDisplayActive ? Theme.accent : Theme.danger
        description: "Technical adapter and fallback details are available when troubleshooting capture output."
        SettingsDisclosure {
            label: "Technical HDR details"
            Repeater {
                model: root.hdrDetailItems()
                delegate: RowLayout {
                    required property var modelData
                    Layout.fillWidth: true
                    Layout.leftMargin: Theme.s4
                    spacing: Theme.s12

                    Text {
                        text: "\u2022"
                        color: Theme.accent
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontBody
                        Layout.alignment: Qt.AlignTop
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: Theme.s4
                        Text {
                            visible: modelData.label.length > 0
                            text: modelData.label
                            color: Theme.text
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.fontCaption
                            font.weight: Font.DemiBold
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }
                        Text {
                            text: modelData.value
                            color: Theme.textMuted
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.fontCaption
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }
                    }
                }
            }
        }
    }

    SettingsSection {
        eyebrow: "Recovery"
        title: "Restore"
        variant: "compact"
        description: "Restoring settings never deletes captures, favorites, watched media, or database records."
        SettingsDisclosure {
            label: "Restore options"
            SettingsRow {
                label: "Restore one category"
                description: "Return only the selected category to its defaults."
                controlWidth: Theme.s48 * 9
                AccentButton { label: "General"; quiet: true; onClicked: root.restoreCategoryRequested("General") }
                AccentButton { label: "Capture"; quiet: true; onClicked: root.restoreCategoryRequested("Capture") }
                AccentButton { label: "Replay"; quiet: true; onClicked: root.restoreCategoryRequested("Replay") }
                AccentButton { label: "Feedback"; quiet: true; onClicked: root.restoreCategoryRequested("Notifications & Sound") }
            }
            SettingsRow {
                label: "Restore input bindings"
                description: "Return controller, keyboard, and mouse overrides to built-in defaults."
                AccentButton { label: "Restore input"; quiet: true; onClicked: root.restoreInputRequested() }
            }
            SettingsRow {
                tone: "danger"
                label: "Restore all settings"
                description: "Return every configuration category to its default values."
                showDivider: false
                AccentButton {
                    label: "Restore all"
                    quiet: true
                    labelColor: Theme.danger
                    borderColor: Theme.danger
                    onClicked: root.restoreAllRequested()
                }
            }
        }
    }

    FolderDialog {
        id: portableFolderDialog
        title: "Select the GameHQ portable folder"
        onAccepted: root.importPortableRequested(selectedFolder)
    }
}
