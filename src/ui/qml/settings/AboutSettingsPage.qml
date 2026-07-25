import QtQuick
import QtQuick.Layouts
import GameHQ
import "../components"

SettingsPage {
    pageTitle: "About"
    pageDescription: "Version, update status, project resources, and ways to help."

    SettingsSection {
        eyebrow: "Application"
        title: Brand.name
        status: "Version " + app.version
        description: app.portableMode ? "Portable profile" : "Installed profile"
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
                Layout.fillWidth: true
                spacing: Theme.s4
                Text {
                    text: updates.stateName === "UpdateAvailable"
                          ? "GameHQ " + updates.latestVersion + " is available"
                          : "GameHQ is ready"
                    color: Theme.text
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontH3
                    font.weight: Font.DemiBold
                }
                Text {
                    text: "Free and open source under the MIT License."
                    color: Theme.textMuted
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontBody
                }
            }
        }
    }

    SettingsSection {
        eyebrow: "Maintenance"
        title: "Updates"
        status: updates.stateName === "UpdateAvailable" ? "Available" : "Current"
        description: {
            if (updates.stateName === "Checking") return "Checking for updates..."
            if (updates.stateName === "UpdateAvailable") return "GameHQ " + updates.latestVersion + " is available."
            if (updates.stateName === "Downloading") return "Downloading GameHQ " + updates.latestVersion + "... " + updates.progress + "%"
            if (updates.stateName === "ReadyToInstall") return "GameHQ " + updates.latestVersion + " is downloaded and SHA-256 verified."
            if (updates.stateName === "PreparingForUpdate" || updates.stateName === "Quiescent") return "Getting ready to install GameHQ " + updates.latestVersion + "..."
            if (updates.stateName === "Installing") return "Installing GameHQ " + updates.latestVersion + "..."
            if (updates.stateName === "Failed" && updates.errorText !== "") return updates.errorText
            if (updates.lastChecked.getTime() > 0) return "Up to date, last checked " + Qt.formatDateTime(updates.lastChecked, "d MMM yyyy, HH:mm")
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
                id: checkButton
                label: updates.stateName === "Checking" ? "Checking..." : "Check now"
                primary: !["UpdateAvailable", "Downloading", "ReadyToInstall", "PreparingForUpdate",
                           "Quiescent", "Installing", "Failed"].includes(updates.stateName)
                quiet: !primary
                enabled: updates.stateName !== "Checking" && updates.stateName !== "Downloading"
                onClicked: updates.checkNow()
            }
        }
        SettingsRow {
            visible: ["UpdateAvailable", "Downloading", "ReadyToInstall", "PreparingForUpdate",
                      "Quiescent", "Installing", "Failed"].includes(updates.stateName)
            label: {
                switch (updates.stateName) {
                case "Downloading": return "Download progress"
                case "ReadyToInstall": return "Ready to install"
                case "PreparingForUpdate":
                case "Quiescent":
                case "Installing": return "Installing"
                default: return "Beta update download"
                }
            }
            description: {
                switch (updates.stateName) {
                case "Downloading": return updates.progress + "% complete"
                case "ReadyToInstall": return "GameHQ will restart to apply the update."
                case "PreparingForUpdate":
                case "Quiescent": return "Waiting for capture work to finish safely..."
                case "Installing": return "GameHQ is applying the update and will restart."
                default: return "SHA-256 detects corruption, but not a compromised GitHub account."
                }
            }
            AccentButton {
                visible: !["PreparingForUpdate", "Quiescent", "Installing"].includes(updates.stateName)
                label: {
                    switch (updates.stateName) {
                    case "Downloading": return "Cancel"
                    case "ReadyToInstall": return "Install and restart"
                    case "Failed": return updates.failedDuringCheck ? "Check again" : "Retry download"
                    default: return "Download"
                    }
                }
                primary: updates.stateName !== "Downloading"
                quiet: updates.stateName === "Downloading"
                onClicked: {
                    switch (updates.stateName) {
                    case "Downloading": updates.cancelDownload(); break
                    case "ReadyToInstall": updates.installAndRestart(); break
                    case "Failed": updates.failedDuringCheck ? updates.checkNow() : updates.downloadUpdate(); break
                    default: updates.downloadUpdate()
                    }
                }
            }
        }
        SettingsLinkRow {
            visible: updates.latestVersion !== ""
            icon: "\u2197"
            label: "View release notes"
            description: Brand.releasesUrl
            showDivider: false
            onClicked: updates.openReleasePage()
        }
    }

    SettingsSection {
        eyebrow: "Open source"
        title: "Project"
        description: "Open official GameHQ resources in your default browser."
        SettingsLinkRow { icon: "\u2302"; label: "Website"; description: Brand.websiteUrl; onClicked: Qt.openUrlExternally(Brand.websiteUrl) }
        SettingsLinkRow { icon: "\u2197"; label: "Source on GitHub"; description: Brand.repositoryUrl; onClicked: Qt.openUrlExternally(Brand.repositoryUrl) }
        SettingsLinkRow { icon: "\u21BB"; label: "Releases"; description: Brand.releasesUrl; onClicked: Qt.openUrlExternally(Brand.releasesUrl) }
        SettingsLinkRow { icon: "!"; label: "Report an issue"; description: Brand.issuesUrl; onClicked: Qt.openUrlExternally(Brand.issuesUrl) }
        SettingsLinkRow { icon: "\u26E8"; label: "Security & privacy"; description: "Verification, local data, network use, and private reporting"; onClicked: Qt.openUrlExternally(Brand.securityUrl) }
        SettingsLinkRow { icon: "\u00A7"; label: "MIT License"; description: Brand.repositoryUrl + "/blob/main/LICENSE"; showDivider: false; onClicked: Qt.openUrlExternally(Brand.repositoryUrl + "/blob/main/LICENSE") }
    }

    SettingsSection {
        eyebrow: "Community"
        title: "Support the project"
        variant: "compact"
        description: "Enjoying " + Brand.name + "? A GitHub star helps more players discover the project."
        AccentButton {
            label: "Star " + Brand.name + " on GitHub"
            quiet: true
            onClicked: Qt.openUrlExternally(Brand.repositoryUrl)
        }
    }
}
