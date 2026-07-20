import QtQuick
import QtQuick.Layouts
import GameHQ
import "../components"

SettingsPage {
    SettingsSection {
        title: "Application"
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.s16
            Image {
                source: "qrc:/icons/gamehq.svg"
                Layout.preferredWidth: Theme.s48
                Layout.preferredHeight: Theme.s48
                sourceSize.width: Theme.s48
                sourceSize.height: Theme.s48
            }
            ColumnLayout {
                spacing: Theme.s4
                Text {
                    text: Brand.name
                    color: Theme.text
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontTitle
                    font.bold: true
                }
                Text {
                    text: "Version " + app.version
                    color: Theme.textMuted
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontBody
                }
            }
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
        title: "Updates"
        description: {
            if (updates.stateName === "Checking") return "Checking for updates..."
            if (updates.stateName === "UpdateAvailable") return "GameHQ " + updates.latestVersion + " is available."
            if (updates.stateName === "Downloading") return "Downloading GameHQ " + updates.latestVersion + "... " + updates.progress + "%"
            if (updates.stateName === "ReadyToInstall") return "GameHQ " + updates.latestVersion + " is downloaded and SHA-256 verified."
            if (updates.stateName === "Failed" && updates.errorText !== "") return updates.errorText
            if (updates.lastChecked.getTime() > 0)
                return "Up to date, last checked " + Qt.formatDateTime(updates.lastChecked, "d MMM yyyy, HH:mm")
            return "GameHQ can check GitHub for newer stable releases."
        }
        SettingsRow {
            label: "Check automatically"
            description: "At most once every 24 hours, in the background."
            SettingsToggle { configKey: "updates.check_automatically"; defaultValue: true }
        }
        SettingsRow {
            label: "Check for updates"
            description: updates.stateName === "UpdateAvailable" ? updates.latestVersion + " available" : "Installed: " + app.version
            AccentButton {
                label: updates.stateName === "Checking" ? "Checking..." : "Check now"
                primary: true
                enabled: updates.stateName !== "Checking" && updates.stateName !== "Downloading"
                onClicked: updates.checkNow()
            }
        }
        SettingsRow {
            visible: updates.stateName === "UpdateAvailable" || updates.stateName === "Downloading" || updates.stateName === "Failed"
            label: updates.stateName === "Downloading" ? "Download progress" : "Beta update download"
            description: updates.stateName === "Downloading"
                         ? updates.progress + "% complete"
                         : "SHA-256 detects corruption, but not a compromised GitHub account."
            AccentButton {
                label: updates.stateName === "Downloading" ? "Cancel"
                       : updates.stateName === "Failed" ? "Retry" : "Download"
                primary: true
                onClicked: updates.stateName === "Downloading" ? updates.cancelDownload() : updates.downloadUpdate()
            }
        }
        SettingsRow {
            visible: updates.latestVersion !== ""
            label: "View release"
            description: Brand.releasesUrl
            AccentButton { label: "Open"; onClicked: updates.openReleasePage() }
        }
    }
    SettingsSection {
        title: "Project"
        SettingsRow {
            label: "Website"
            description: Brand.websiteUrl
            AccentButton { label: "Open"; primary: true; onClicked: Qt.openUrlExternally(Brand.websiteUrl) }
        }
        SettingsRow {
            label: "Source on GitHub"
            description: Brand.repositoryUrl
            AccentButton { label: "Open"; primary: true; onClicked: Qt.openUrlExternally(Brand.repositoryUrl) }
        }
        SettingsRow {
            label: "Releases"
            description: Brand.releasesUrl
            AccentButton { label: "Open"; primary: true; onClicked: Qt.openUrlExternally(Brand.releasesUrl) }
        }
        SettingsRow {
            label: "Report an issue"
            description: Brand.issuesUrl
            AccentButton { label: "Open"; primary: true; onClicked: Qt.openUrlExternally(Brand.issuesUrl) }
        }
        SettingsRow {
            label: "License"
            description: Brand.repositoryUrl + "/blob/main/LICENSE.txt"
            AccentButton { label: "Open"; primary: true; onClicked: Qt.openUrlExternally(Brand.repositoryUrl + "/blob/main/LICENSE.txt") }
        }
    }
    SettingsSection {
        title: "Support the project"
        description: "Enjoying " + Brand.name + "? A GitHub star helps more people discover the project."
        AccentButton { label: "Star " + Brand.name + " on GitHub"; primary: true; onClicked: Qt.openUrlExternally(Brand.repositoryUrl) }
    }
}
