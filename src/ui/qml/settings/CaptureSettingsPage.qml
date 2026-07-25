import QtQuick
import QtQuick.Dialogs
import QtQuick.Layouts
import GameHQ
import "../components"

SettingsPage {
    id: root
    pageTitle: "Capture"
    pageDescription: "Choose when, how, and where GameHQ saves screenshots."
    property string locationError: ""

    function finishLocationChange(error) {
        locationError = error
        sounds.play(error.length > 0 ? "error" : "confirm")
    }

    function openFolder(path) {
        Qt.openUrlExternally("file:///" + path.replace(/\\/g, "/"))
    }

    SettingsSection {
        eyebrow: "Capture mode"
        title: "When to capture"
        description: "Control when screenshots and replay recording are allowed."
        SettingsRow {
            label: "Capture mode"
            description: "Only in games is the safest default for global shortcuts."
            SettingsCombo {
                configKey: "capture.mode"
                defaultValue: "only_in_games"
                options: [
                    { label: "Only in games", value: "only_in_games" },
                    { label: "Whitelisted games", value: "whitelist" },
                    { label: "Always", value: "always" }
                ]
            }
        }
    }

    SettingsSection {
        eyebrow: "Image"
        title: "Format and quality"
        description: "PNG is lossless; JPEG trades some quality for smaller files."
        SettingsRow {
            label: "Format"
            SettingsCombo {
                id: formatCombo
                configKey: "capture.screenshot_format"
                defaultValue: "png"
                options: [
                    { label: "PNG (lossless)", value: "png" },
                    { label: "JPEG (smaller files)", value: "jpg" }
                ]
            }
        }
        SettingsRow {
            label: "JPEG quality"
            visible: formatCombo.currentIndex >= 0
                     && formatCombo.options[formatCombo.currentIndex].value === "jpg"
            SettingsCombo {
                configKey: "capture.jpeg_quality"
                defaultValue: 90
                options: [
                    { label: "70%", value: 70 }, { label: "80%", value: 80 },
                    { label: "90%", value: 90 }, { label: "100%", value: 100 }
                ]
            }
        }
    }

    SettingsSection {
        eyebrow: "Feedback"
        title: "After a screenshot"
        description: "Combined with the master switches on the Notifications & Sound page."
        SettingsRow {
            label: "Screenshot sound"
            SettingsToggle { configKey: "capture.screenshot_sound"; defaultValue: true }
        }
        SettingsRow {
            label: "Screenshot notification"
            SettingsToggle { configKey: "capture.screenshot_notify"; defaultValue: true }
        }
    }

    SettingsSection {
        eyebrow: "Storage"
        title: "Where captures are saved"
        description: "Changing a location never moves or deletes existing media."

        SettingsPathRow {
            label: "Screenshots"
            path: app.screenshotsRoot
            showChange: true
            showReset: app.screenshotsRoot !== app.capturesRoot
            onChangeRequested: screenshotFolderDialog.open()
            onOpenRequested: root.openFolder(app.screenshotsRoot)
            onResetRequested: root.finishLocationChange(app.resetCaptureRoot("screenshots"))
        }

        SettingsPathRow {
            label: "Replay clips"
            path: app.clipsRoot
            showChange: true
            showReset: app.clipsRoot !== app.capturesRoot
            showDivider: false
            onChangeRequested: clipFolderDialog.open()
            onOpenRequested: root.openFolder(app.clipsRoot)
            onResetRequested: root.finishLocationChange(app.resetCaptureRoot("clips"))
        }

        Text {
            visible: root.locationError.length > 0
            text: root.locationError
            color: Theme.danger
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontCaption
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
    }

    FolderDialog {
        id: screenshotFolderDialog
        title: "Choose the screenshots folder"
        onAccepted: root.finishLocationChange(app.setCaptureRoot("screenshots", selectedFolder))
    }

    FolderDialog {
        id: clipFolderDialog
        title: "Choose the clips folder"
        onAccepted: root.finishLocationChange(app.setCaptureRoot("clips", selectedFolder))
    }
}
