import QtQuick
import QtQuick.Controls.Basic as QC
import QtQuick.Layouts
import GameHQ

// Compact, communication-focused About surface. The detailed update controls,
// storage mode and full project link list deliberately remain in Settings.
Item {
    id: root

    property bool postUpdateGreeting: false
    property int padIndex: 0
    signal closed(bool wasPostUpdateGreeting)
    signal updateSettingsRequested()

    visible: false
    opacity: 0
    focus: visible

    function hasUpdateRelease() {
        return updates.latestVersion !== ""
            && ["UpdateAvailable", "Downloading", "ReadyToInstall", "PreparingForUpdate",
                "Quiescent", "Installing", "Failed"].includes(updates.stateName)
    }

    function displayedVersion() {
        return hasUpdateRelease() ? updates.latestVersion : app.version
    }

    function updateStatus() {
        switch (updates.stateName) {
        case "Checking": return "Checking for updates"
        case "UpdateAvailable": return "Update available"
        case "Downloading": return "Downloading · " + updates.progress + "%"
        case "ReadyToInstall": return "Ready to install"
        case "PreparingForUpdate":
        case "Quiescent": return "Preparing to install"
        case "Installing": return "Installing"
        case "Failed": return "Update check failed"
        case "UpToDate": return "Up to date"
        default: return "Ready to check"
        }
    }

    function primaryLabel() {
        switch (updates.stateName) {
        case "Checking": return "Checking..."
        case "UpdateAvailable": return "Download update"
        case "Downloading": return "Cancel download"
        case "ReadyToInstall": return "Install and restart"
        case "PreparingForUpdate":
        case "Quiescent": return "Preparing..."
        case "Installing": return "Installing..."
        case "Failed": return updates.failedDuringCheck ? "Check again" : "Retry download"
        default: return "Check for updates"
        }
    }

    function primaryEnabled() {
        return !["Checking", "PreparingForUpdate", "Quiescent", "Installing"].includes(updates.stateName)
    }

    function runPrimaryAction() {
        switch (updates.stateName) {
        case "UpdateAvailable": updates.downloadUpdate(); break
        case "Downloading": updates.cancelDownload(); break
        case "ReadyToInstall": updates.installAndRestart(); break
        case "Failed": updates.failedDuringCheck ? updates.checkNow() : updates.downloadUpdate(); break
        case "Checking":
        case "PreparingForUpdate":
        case "Quiescent":
        case "Installing": break
        default: updates.checkNow()
        }
    }

    function remoteNotesPreview() {
        const notes = updates.notes || ""
        return notes.length > 2500 ? notes.slice(0, 2500) + "\n…" : notes
    }

    function focusableButtons() {
        return [primaryAction, fullNotesButton, githubButton, issueButton,
                settingsButton, closeButton].filter(button => button.visible && button.enabled)
    }

    function focusPadIndex() {
        const buttons = focusableButtons()
        if (buttons.length === 0)
            return
        padIndex = Math.max(0, Math.min(buttons.length - 1, padIndex))
        buttons[padIndex].forceActiveFocus()
    }

    function padStep(direction) {
        const buttons = focusableButtons()
        if (buttons.length === 0)
            return
        let current = buttons.findIndex(button => button.activeFocus)
        if (current < 0)
            current = padIndex
        padIndex = (current + direction + buttons.length) % buttons.length
        focusPadIndex()
        sounds.play("nav_tick")
    }

    function padConfirm() {
        const buttons = focusableButtons()
        const current = buttons.find(button => button.activeFocus)
        if (current)
            current.clicked()
    }

    function open(asPostUpdateGreeting) {
        postUpdateGreeting = !!asPostUpdateGreeting
        visible = true
        notesFlick.contentY = 0
        padIndex = 0
        Qt.callLater(focusPadIndex)
    }

    function close() {
        if (!visible)
            return
        const wasPostUpdate = postUpdateGreeting
        visible = false
        postUpdateGreeting = false
        closed(wasPostUpdate)
    }

    Behavior on opacity { NumberAnimation { duration: Theme.durFast } }
    states: State {
        when: root.visible
        PropertyChanges { target: root; opacity: 1 }
    }

    Keys.priority: Keys.BeforeItem
    Keys.onEscapePressed: function(event) {
        event.accepted = true
        root.close()
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.scrim
        MouseArea { anchors.fill: parent; onClicked: root.close() }
    }

    Rectangle {
        id: card
        anchors.centerIn: parent
        width: Math.min(720, root.width - Theme.s48)
        height: Math.min(620, root.height - Theme.s48)
        radius: Theme.radiusL
        color: Theme.surface
        border.width: 1
        border.color: Theme.stroke
        clip: true

        MouseArea { anchors.fill: parent }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: Theme.s24
            spacing: Theme.s16

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
                        text: root.postUpdateGreeting
                              ? Brand.name + " was updated to " + app.version
                              : Brand.name
                        color: Theme.text
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontTitle
                        font.weight: Font.DemiBold
                    }
                    Text {
                        text: "Version " + app.version + " · "
                              + (app.portableMode ? "Portable" : "Installed")
                              + " · " + root.updateStatus()
                        color: Theme.textMuted
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontBody
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                visible: updates.stateName === "Failed" && updates.errorText !== ""
                implicitHeight: errorText.implicitHeight + Theme.s16 * 2
                radius: Theme.radiusM
                color: Theme.bg1
                border.width: 1
                border.color: Theme.danger
                Text {
                    id: errorText
                    anchors.fill: parent
                    anchors.margins: Theme.s16
                    text: updates.errorText
                    textFormat: Text.PlainText
                    color: Theme.textMuted
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontBody
                    wrapMode: Text.WordWrap
                }
            }

            Flickable {
                id: notesFlick
                Layout.fillWidth: true
                Layout.fillHeight: true
                contentWidth: width
                contentHeight: notesColumn.implicitHeight
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                QC.ScrollBar.vertical: QC.ScrollBar {}

                ColumnLayout {
                    id: notesColumn
                    width: notesFlick.width - Theme.s12
                    spacing: Theme.s12

                    Text {
                        Layout.fillWidth: true
                        text: "WHAT'S NEW IN " + root.displayedVersion()
                        color: Theme.textFaint
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontCaption
                        font.letterSpacing: Theme.letterSpacingWide
                    }

                    Text {
                        Layout.fillWidth: true
                        visible: root.hasUpdateRelease() && updates.notes !== ""
                        text: root.remoteNotesPreview()
                        textFormat: Text.PlainText
                        color: Theme.textMuted
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontBody
                        wrapMode: Text.WordWrap
                    }

                    Repeater {
                        model: root.hasUpdateRelease() ? [] : app.releaseNotesSections
                        delegate: ColumnLayout {
                            required property var modelData
                            Layout.fillWidth: true
                            spacing: Theme.s4
                            Text {
                                text: modelData.title
                                color: Theme.text
                                font.family: Theme.fontFamily
                                font.pixelSize: Theme.fontBody
                                font.weight: Font.DemiBold
                            }
                            Repeater {
                                model: modelData.items
                                delegate: Text {
                                    required property string modelData
                                    Layout.fillWidth: true
                                    text: "• " + modelData
                                    textFormat: Text.PlainText
                                    color: Theme.textMuted
                                    font.family: Theme.fontFamily
                                    font.pixelSize: Theme.fontBody
                                    wrapMode: Text.WordWrap
                                }
                            }
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: "ABOUT " + Brand.name.toUpperCase()
                        color: Theme.textFaint
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontCaption
                        font.letterSpacing: Theme.letterSpacingWide
                        Layout.topMargin: Theme.s8
                    }
                    Text {
                        Layout.fillWidth: true
                        text: "A controller-friendly screenshot, replay, and media gallery for PC games."
                        textFormat: Text.PlainText
                        color: Theme.textMuted
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontBody
                        wrapMode: Text.WordWrap
                    }
                }
            }

            Flow {
                Layout.fillWidth: true
                spacing: Theme.s8
                implicitHeight: childrenRect.height
                Layout.preferredHeight: implicitHeight

                AccentButton {
                    id: primaryAction
                    label: root.primaryLabel()
                    primary: true
                    enabled: root.primaryEnabled()
                    onClicked: root.runPrimaryAction()
                }
                AccentButton {
                    id: fullNotesButton
                    label: "Full release notes"
                    onClicked: {
                        if (root.hasUpdateRelease() && updates.releaseUrl !== "")
                            updates.openReleasePage()
                        else
                            Qt.openUrlExternally(Brand.releasesUrl + "/tag/v" + app.version)
                    }
                }
                AccentButton {
                    id: githubButton
                    label: "GitHub"
                    onClicked: Qt.openUrlExternally(Brand.repositoryUrl)
                }
                AccentButton {
                    id: issueButton
                    label: "Report issue"
                    onClicked: Qt.openUrlExternally(Brand.issuesUrl)
                }
                AccentButton {
                    id: settingsButton
                    label: "Update settings"
                    onClicked: root.updateSettingsRequested()
                }
                AccentButton {
                    id: closeButton
                    label: "Close"
                    onClicked: root.close()
                }
            }
        }
    }
}
