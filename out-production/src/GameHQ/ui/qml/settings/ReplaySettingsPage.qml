import QtQuick
import GameHQ
import "../components"

SettingsPage {
    pageTitle: "Replay"
    pageDescription: "Manage the rolling buffer used for instant replay clips."

    SettingsSection {
        eyebrow: "Current status"
        title: "Replay buffer"
        variant: "status"
        status: app.replayBufferActive ? "Recording" : "Idle"
        description: app.replayBufferActive
            ? "Recording " + app.replayBufferGame + "; the temporary ring is written only when you save a replay."
            : "Not recording. The buffer arms automatically when an eligible game is active."
    }

    SettingsSection {
        eyebrow: "Buffer"
        title: "Automatic recording"
        description: "Recording changes restart an active buffer so new values apply immediately."
        SettingsRow {
            label: "Automatic buffer"
            description: "Record a rolling buffer whenever an eligible game is active."
            SettingsToggle { configKey: "replay.auto"; defaultValue: true }
        }
        SettingsRow {
            label: "Replay length"
            showDivider: false
            SettingsCombo {
                configKey: "replay.length_seconds"; defaultValue: 300
                options: [
                    { label: "30 seconds", value: 30 }, { label: "1 minute", value: 60 },
                    { label: "3 minutes", value: 180 }, { label: "5 minutes", value: 300 },
                    { label: "10 minutes", value: 600 }, { label: "15 minutes", value: 900 }
                ]
            }
        }
    }

    SettingsSection {
        eyebrow: "Encoding"
        title: "Recording quality"
        description: "Balance motion detail, resolution, storage use, and encoder load."
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
            description: "Higher values improve motion detail but use more storage and encoder bandwidth."
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
            showDivider: false
            SettingsToggle { configKey: "audio.enabled"; defaultValue: false }
        }
    }

    SettingsSection {
        eyebrow: "Feedback"
        title: "After saving a clip"
        description: "Saved replays go to " + app.clipsRoot + ". Failures always notify you."
        SettingsRow {
            label: "Clip saved sound"
            SettingsToggle { configKey: "replay.clip_sound"; defaultValue: true }
        }
        SettingsRow {
            label: "Clip saved notification"
            showDivider: false
            SettingsToggle { configKey: "replay.clip_notify"; defaultValue: true }
        }
    }
}
