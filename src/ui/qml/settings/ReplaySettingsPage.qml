import QtQuick
import GameHQ
import "../components"

SettingsPage {
    SettingsSection {
        title: "Replay buffer"
        description: "Recording changes restart an active rolling buffer so new values apply immediately. Feedback-only options below never interrupt it."
        SettingsRow {
            label: "Buffer state"
            description: app.replayBufferActive
                ? "Recording a rolling buffer of " + app.replayBufferGame + ". The ring is held in a temporary cache and only written to the clip folder when you save a replay."
                : "Not recording. The buffer arms automatically once an eligible game is in the foreground."
            Text {
                text: app.replayBufferActive ? "● Recording" : "○ Idle"
                color: app.replayBufferActive ? Theme.success : Theme.textMuted
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontBody
            }
        }
        SettingsRow {
            label: "Automatic buffer"
            description: "Record a rolling buffer whenever an eligible game is active."
            SettingsToggle { configKey: "replay.auto"; defaultValue: true }
        }
        SettingsRow {
            label: "Replay length"
            SettingsCombo {
                configKey: "replay.length_seconds"; defaultValue: 300
                options: [
                    { label: "30 seconds", value: 30 }, { label: "1 minute", value: 60 },
                    { label: "3 minutes", value: 180 }, { label: "5 minutes", value: 300 },
                    { label: "10 minutes", value: 600 }, { label: "15 minutes", value: 900 }
                ]
            }
        }
        SettingsRow {
            label: "Frame rate"
            SettingsCombo {
                configKey: "replay.fps"; defaultValue: 30
                options: [{ label: "30 fps", value: 30 }, { label: "60 fps", value: 60 }]
            }
        }
        SettingsRow {
            label: "Resolution"
            SettingsCombo {
                configKey: "replay.resolution"; defaultValue: "1920x1080"
                options: [
                    { label: "720p", value: "1280x720" },
                    { label: "1080p", value: "1920x1080" },
                    { label: "4K", value: "3840x2160" }
                ]
            }
        }
        SettingsRow {
            label: "Video bitrate"
            description: "Higher values improve motion detail but use more disk space and encoder bandwidth."
            SettingsCombo {
                configKey: "replay.bitrate_mbps"; defaultValue: 14
                options: [
                    { label: "8 Mbps", value: 8 },
                    { label: "14 Mbps", value: 14 },
                    { label: "20 Mbps", value: 20 },
                    { label: "35 Mbps", value: 35 }
                ]
            }
        }
        SettingsRow {
            label: "System audio"
            description: "Include desktop audio in newly recorded replay segments."
            SettingsToggle { configKey: "audio.enabled"; defaultValue: false }
        }
    }

    SettingsSection {
        title: "Clip feedback"
        description: "Saved replays are written to " + app.clipsRoot + ". That is separate from the rolling buffer's temporary cache above, which is discarded if it is never saved. Combined with the master switches on the Notifications & Sound page. Save failures always play a sound and notify, regardless of these switches."
        SettingsRow {
            label: "Clip saved sound"
            SettingsToggle { configKey: "replay.clip_sound"; defaultValue: true }
        }
        SettingsRow {
            label: "Clip saved notification"
            SettingsToggle { configKey: "replay.clip_notify"; defaultValue: true }
        }
    }
}
