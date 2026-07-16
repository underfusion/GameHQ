import QtQuick
import GameHQ
import "../components"

SettingsPage {
    SettingsSection {
        title: "Appearance"
        description: "Choose the color scheme " + Brand.name + " uses. The change applies immediately."
        SettingsRow {
            label: "Theme"
            description: "High contrast trades the tinted palette for maximum legibility."
            SettingsCombo {
                configKey: "theme.active_skin"
                defaultValue: "dark"
                options: Theme.availableSkins.map(function (s) {
                    return { label: s.label, value: s.key }
                })
            }
        }
    }

    SettingsSection {
        title: "Startup"
        description: "Choose how " + Brand.name + " starts and whether it follows your Windows sign-in."
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
        title: "Window behavior"
        description: "Choose how " + Brand.name + " behaves when its main window is minimized or closed."
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
