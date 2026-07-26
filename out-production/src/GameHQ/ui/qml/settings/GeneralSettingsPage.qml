import QtQuick
import GameHQ
import "../components"

SettingsPage {
    pageTitle: "General"
    pageDescription: "Appearance, startup, and window behavior."

    SettingsSection {
        eyebrow: "Personalization"
        title: "Look and feel"
        description: "Choose the visual style and in-game overlay strength."
        SettingsRow {
            id: themeRow
            label: "Theme"
            // Track the picker rather than the live skin: this should describe
            // what is selected in the combo, which is the same thing, but the
            // intent is the selection, not whatever is currently painting.
            description: {
                const match = Theme.availableSkins.filter(function (s) {
                    return s.key === Theme.activeSkin
                })
                return match.length ? match[0].blurb
                                    : "Choose how " + Brand.name + " looks."
            }
            SettingsCombo {
                configKey: "theme.active_skin"
                defaultValue: "obsidian"
                options: Theme.availableSkins.map(function (s) {
                    return { label: s.label, value: s.key }
                })
            }
        }
        SettingsRow {
            label: "Overlay dimming"
            description: "How strongly the in-game overlay darkens the game behind it. 100% is the theme's own dimming; lower keeps more of the game visible."
            SettingsSlider {
                configKey: "theme.overlay_scrim_strength"
                defaultValue: 100
                from: 25
                to: 150
                stepSize: 5
            }
        }
    }

    SettingsSection {
        eyebrow: "Startup"
        title: "How " + Brand.name + " starts"
        description: "Choose whether GameHQ follows your Windows sign-in and opens quietly."
        SettingsRow {
            label: "Start with Windows"
            description: "Register " + Brand.name + " for the current Windows user; no administrator access is required."
            SettingsToggle { configKey: "startup.enabled"; defaultValue: false }
        }
        SettingsRow {
            label: "Launch minimized"
            description: "Launch directly in the system tray without opening the main window."
            SettingsToggle { configKey: "startup.minimized"; defaultValue: false }
        }
    }

    SettingsSection {
        eyebrow: "Desktop"
        title: "Window and tray behavior"
        description: "Choose what happens when the main window is minimized or closed."
        SettingsRow {
            label: "Minimize to tray"
            description: "Minimizing the window drops it straight to the tray instead of the taskbar."
            SettingsToggle { configKey: "tray.minimize_to_tray"; defaultValue: false }
        }
        SettingsRow {
            label: "Close to tray"
            description: "Keep capture and replay services running; when disabled, Close exits " + Brand.name + "."
            SettingsToggle { configKey: "tray.close_to_tray"; defaultValue: true }
        }
    }
}
