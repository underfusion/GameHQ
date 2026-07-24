import QtQuick
import GameHQ
import "../components"

SettingsPage {
    pageTitle: "Notifications & Sound"
    pageDescription: "Choose how GameHQ confirms captures, clips, and navigation."

    SettingsSection {
        eyebrow: "Visual feedback"
        title: "Notifications"
        description: "Control the result cards shown after capture and replay actions."
        SettingsRow {
            icon: "\u25A3"
            label: "Show notifications"
            description: "Master switch for capture and replay result cards."
            SettingsToggle { configKey: "notifications.enabled"; defaultValue: true }
        }
        SettingsRow {
            icon: "\u25A3"
            label: "Screenshot captured"
            SettingsToggle { configKey: "capture.screenshot_notify"; defaultValue: true }
        }
        SettingsRow {
            icon: "\u21BA"
            label: "Replay saved"
            showDivider: false
            SettingsToggle { configKey: "replay.clip_notify"; defaultValue: true }
        }
    }

    SettingsSection {
        eyebrow: "Audio feedback"
        title: "Sound"
        description: "Set the master sound switch, volume, and event-specific feedback."
        SettingsRow {
            icon: "\u266B"
            label: "UI sounds"
            description: "Play navigation and action feedback sounds."
            SettingsToggle { configKey: "sounds.enabled"; defaultValue: true }
        }
        SettingsRow {
            label: "Volume"
            SettingsSlider {
                configKey: "sounds.volume"
                defaultValue: 80
                from: 0
                to: 100
                stepSize: 5
            }
        }
        SettingsRow {
            label: "Screenshot sound"
            SettingsToggle { configKey: "capture.screenshot_sound"; defaultValue: true }
        }
        SettingsRow {
            label: "Replay saved sound"
            showDivider: false
            SettingsToggle { configKey: "replay.clip_sound"; defaultValue: true }
        }
    }

    SettingsSection {
        eyebrow: "Preview"
        title: "Test the current feedback settings"
        variant: "compact"
        description: "Confirm notifications and sound without creating a capture."
        SettingsRow {
            label: "Preview feedback"
            showDivider: false
            controlWidth: Theme.s48 * 6
            AccentButton {
                label: "Show test notification"
                quiet: true
                onClicked: notifications.post(Brand.name + " notification", "Notifications are working.", "", "info")
            }
            AccentButton {
                label: "Play test sound"
                primary: true
                onClicked: sounds.play("confirm")
            }
        }
    }
}
