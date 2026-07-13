import QtQuick
import GameHQ
import "../components"

SettingsPage {
    SettingsSection {
        title: "Notifications"
        SettingsRow {
            label: "Show notifications"
            description: "Display capture and replay result cards."
            SettingsToggle { configKey: "notifications.enabled"; defaultValue: true }
        }
    }
    SettingsSection {
        title: "Sound"
        SettingsRow {
            label: "UI sounds"
            description: "Play navigation and capture feedback sounds."
            SettingsToggle { configKey: "sounds.enabled"; defaultValue: true }
        }
        SettingsRow {
            label: "Volume"
            SettingsCombo {
                configKey: "sounds.volume"; defaultValue: 80
                options: [
                    { label: "25%", value: 25 }, { label: "50%", value: 50 },
                    { label: "75%", value: 75 }, { label: "80%", value: 80 },
                    { label: "100%", value: 100 }
                ]
            }
        }
    }
    SettingsSection {
        title: "Capture & replay feedback"
        description: "Per-event choices, combined with the master switches above. Also editable on the Capture and Replay pages. Save failures always play a sound and notify, regardless of these switches."
        SettingsRow {
            label: "Screenshot sound"
            SettingsToggle { configKey: "capture.screenshot_sound"; defaultValue: true }
        }
        SettingsRow {
            label: "Screenshot notification"
            SettingsToggle { configKey: "capture.screenshot_notify"; defaultValue: true }
        }
        SettingsRow {
            label: "Clip saved sound"
            SettingsToggle { configKey: "replay.clip_sound"; defaultValue: true }
        }
        SettingsRow {
            label: "Clip saved notification"
            SettingsToggle { configKey: "replay.clip_notify"; defaultValue: true }
        }
    }
    SettingsSection {
        title: "Preview feedback"
        description: "Confirm the current feedback settings without creating a capture."
        SettingsRow {
            label: "Notification preview"
            description: "Shows one non-activating test card."
            AccentButton {
                label: "Show test"
                primary: true
                onClicked: notifications.post(Brand.name + " notification", "Notifications are working.", "", "info")
            }
        }
        SettingsRow {
            label: "Sound preview"
            description: "Uses the current UI sound toggle and volume."
            AccentButton { label: "Play test"; primary: true; onClicked: sounds.play("confirm") }
        }
    }
}
