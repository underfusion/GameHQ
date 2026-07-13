import QtQuick
import QtQuick.Dialogs
import QtQuick.Layouts
import GameHQ
import "../components"

SettingsPage {
    id: root
    property string locationError: ""

    function finishLocationChange(error) {
        locationError = error
        sounds.play(error.length > 0 ? "error" : "confirm")
    }

    SettingsSection {
        title: "Capture behavior"
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
        title: "Screenshot format"
        description: "JPEG makes smaller files at a visible quality cost; PNG is lossless."
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
        title: "Screenshot feedback"
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
        title: "Capture locations"
        description: "New screenshots and clips can use separate folders. Changing or resetting a location never moves or deletes existing media; " + Brand.name + " keeps scanning previous managed roots."

        SettingsRow {
            label: "Screenshots folder"
            description: "PNG captures are written below this root in <Game>/Screenshots/."
            ColumnLayout {
                Layout.preferredWidth: Theme.s48 * 7
                spacing: Theme.s8
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: screenshotsPathText.implicitHeight + Theme.s8 * 2
                    radius: Theme.radiusM
                    color: Theme.bg1
                    border.width: 1
                    border.color: Theme.borderLight
                    Text {
                        id: screenshotsPathText
                        anchors.fill: parent
                        anchors.margins: Theme.s8
                        text: app.screenshotsRoot
                        color: Theme.textMuted
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontCaption
                        horizontalAlignment: Text.AlignRight
                        elide: Text.ElideMiddle
                    }
                }
                RowLayout {
                    Layout.alignment: Qt.AlignRight
                    spacing: Theme.s8
                    AccentButton {
                        label: "Use default"
                        primary: true
                        visible: app.screenshotsRoot !== app.capturesRoot
                        onClicked: root.finishLocationChange(app.resetCaptureRoot("screenshots"))
                    }
                    AccentButton {
                        label: "Choose..."
                        primary: true
                        onClicked: screenshotFolderDialog.open()
                    }
                }
            }
        }

        Rectangle { // separator between the two folder rows
            Layout.fillWidth: true
            implicitHeight: 1
            color: Theme.borderLight
        }

        SettingsRow {
            label: "Clips folder"
            description: "Saved replay MP4 files are written below this root in <Game>/Clips/."
            ColumnLayout {
                Layout.preferredWidth: Theme.s48 * 7
                spacing: Theme.s8
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: clipsPathText.implicitHeight + Theme.s8 * 2
                    radius: Theme.radiusM
                    color: Theme.bg1
                    border.width: 1
                    border.color: Theme.borderLight
                    Text {
                        id: clipsPathText
                        anchors.fill: parent
                        anchors.margins: Theme.s8
                        text: app.clipsRoot
                        color: Theme.textMuted
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontCaption
                        horizontalAlignment: Text.AlignRight
                        elide: Text.ElideMiddle
                    }
                }
                RowLayout {
                    Layout.alignment: Qt.AlignRight
                    spacing: Theme.s8
                    AccentButton {
                        label: "Use default"
                        primary: true
                        visible: app.clipsRoot !== app.capturesRoot
                        onClicked: root.finishLocationChange(app.resetCaptureRoot("clips"))
                    }
                    AccentButton {
                        label: "Choose..."
                        primary: true
                        onClicked: clipFolderDialog.open()
                    }
                }
            }
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
