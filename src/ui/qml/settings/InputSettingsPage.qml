import QtQuick
import QtQuick.Layouts
import GameHQ
import "../components"

SettingsPage {
    id: root
    readonly property var editor: input.bindingEditor

    SettingsSection {
        title: "Input devices"
        description: "Choose a device type, then select either assignment slot to capture a new input."
        SettingsRow {
            label: "Device type"
            description: editor.deviceGroup === "controller" ? input.controllerStatus
                       : editor.deviceGroup === "keyboard" ? "Focused shortcuts and global key combinations"
                                                           : "Middle, Back, and Forward mouse buttons"
            RowLayout {
                spacing: Theme.s8
                Repeater {
                    model: [
                        { label: "Controller", value: "controller" },
                        { label: "Keyboard", value: "keyboard" },
                        { label: "Mouse", value: "mouse" }
                    ]
                    delegate: AccentButton {
                        label: modelData.label
                        primary: editor.deviceGroup === modelData.value
                        onClicked: editor.deviceGroup = modelData.value
                    }
                }
            }
        }
        SettingsRow {
            visible: editor.deviceGroup === "controller"
            label: "Controller profile"
            description: editor.controllerSpecific
                         ? "Changes apply only to " + editor.controllerName + "."
                         : "Position-based assignments work across PlayStation, Xbox, Nintendo, and generic pads."
            RowLayout {
                AccentButton {
                    label: "All controllers"
                    primary: !editor.controllerSpecific
                    onClicked: editor.controllerSpecific = false
                }
                AccentButton {
                    visible: editor.controllerSpecificAvailable
                    label: editor.controllerName.length > 0 ? editor.controllerName : "This controller"
                    primary: editor.controllerSpecific
                    onClicked: editor.controllerSpecific = true
                }
            }
        }
    }

    SettingsSection {
        title: "Test and restore"
        SettingsRow {
            label: "Last input"
            description: input.lastInput
            Text {
                text: editor.lastFiredAction
                color: Theme.accent
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontBody
            }
        }
        SettingsRow {
            label: "Restore displayed bindings"
            description: editor.controllerSpecific
                         ? "Remove overrides for this controller only."
                         : "Remove overrides for the selected device type and shared profile."
            AccentButton { label: "Restore defaults"; primary: true; onClicked: resetProfileDialog.open() }
        }
    }

    SettingsSection {
        title: "Gesture timing"
        SettingsRow {
            label: "Capture-button hold time"
            description: "A completed hold saves replay and consumes the screenshot tap."
            SettingsCombo {
                configKey: "input.share_hold_ms"; defaultValue: 2000
                options: [
                    { label: "1.0 seconds", value: 1000 }, { label: "1.5 seconds", value: 1500 },
                    { label: "2.0 seconds", value: 2000 }, { label: "3.0 seconds", value: 3000 }
                ]
            }
        }
    }

    SettingsSection {
        title: "Assignments"
        description: "Primary and secondary slots are independent. Contexts can reuse the same input safely."

        GridLayout {
            Layout.fillWidth: true
            columns: 4
            columnSpacing: Theme.s12
            Item { Layout.fillWidth: true }
            Repeater {
                model: [
                    { label: "PRIMARY", width: 210 },
                    { label: "SECONDARY", width: 210 },
                    { label: "ACTION", width: 76 }
                ]
                delegate: Text {
                    text: modelData.label
                    color: Theme.textFaint
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontCaption
                    font.letterSpacing: Theme.letterSpacingWide
                    Layout.preferredWidth: modelData.width
                }
            }
        }

        Repeater {
            model: editor.rows
            delegate: ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.s8

                GridLayout {
                    Layout.fillWidth: true
                    columns: 4
                    columnSpacing: Theme.s12

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: Theme.s4
                        Text {
                            text: modelData.label
                            color: Theme.text
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.fontBody
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }
                        Text {
                            text: modelData.scope + " · " + modelData.description
                            color: Theme.textMuted
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.fontCaption
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }
                    }

                    Rectangle {
                        Layout.preferredWidth: 210
                        Layout.preferredHeight: 44
                        radius: Theme.radiusS
                        color: Theme.bg1
                        border.width: 1
                        border.color: Theme.stroke
                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: Theme.s4
                            spacing: Theme.s4
                            AccentButton {
                                label: modelData.bindable ? modelData.primary : "Fixed · " + modelData.primary
                                quiet: true
                                enabled: modelData.bindable
                                Layout.fillWidth: true
                                Layout.preferredWidth: 0
                                onClicked: editor.beginCapture(modelData.actionId, 1)
                            }
                            AccentButton {
                                label: "×"
                                quiet: true
                                enabled: modelData.bindable && modelData.primary !== "Unassigned"
                                Layout.preferredWidth: 34
                                onClicked: editor.clearBinding(modelData.actionId, 1)
                            }
                        }
                    }

                    Rectangle {
                        Layout.preferredWidth: 210
                        Layout.preferredHeight: 44
                        radius: Theme.radiusS
                        color: Theme.bg1
                        border.width: 1
                        border.color: Theme.stroke
                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: Theme.s4
                            spacing: Theme.s4
                            AccentButton {
                                label: modelData.bindable ? modelData.secondary : "Fixed"
                                quiet: true
                                enabled: modelData.bindable
                                Layout.fillWidth: true
                                Layout.preferredWidth: 0
                                onClicked: editor.beginCapture(modelData.actionId, 2)
                            }
                            AccentButton {
                                label: "×"
                                quiet: true
                                enabled: modelData.bindable && modelData.secondary !== "Unassigned"
                                Layout.preferredWidth: 34
                                onClicked: editor.clearBinding(modelData.actionId, 2)
                            }
                        }
                    }

                    AccentButton {
                        label: "Reset"
                        quiet: true
                        enabled: modelData.bindable
                        Layout.preferredWidth: 76
                        onClicked: editor.resetAction(modelData.actionId)
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    color: Theme.stroke
                    opacity: index < editor.rows.length - 1 ? 1 : 0
                }
            }
        }
    }

    Rectangle {
        parent: root
        anchors.fill: parent
        z: 200
        visible: editor.captureActive
        color: Theme.scrim
        focus: visible
        onVisibleChanged: if (visible) forceActiveFocus()
        Keys.onPressed: (event) => {
            event.accepted = input.handleKeyPressed(event.key, event.modifiers, event.isAutoRepeat)
        }
        Keys.onReleased: (event) => {
            event.accepted = input.handleKeyReleased(event.key, event.modifiers)
        }
        Rectangle {
            anchors.centerIn: parent
            width: Math.min(parent.width - Theme.s48, 560)
            height: captureColumn.implicitHeight + Theme.s24 * 2
            radius: Theme.radiusL
            color: Theme.surface
            border.width: 2
            border.color: Theme.accent
            ColumnLayout {
                id: captureColumn
                anchors.fill: parent
                anchors.margins: Theme.s24
                spacing: Theme.s16
                Text {
                    text: "Waiting for input"
                    color: Theme.text
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontTitle
                }
                Text {
                    text: editor.capturePrompt
                    color: Theme.textMuted
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontBody
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
                AccentButton { label: "Cancel"; onClicked: editor.cancelCapture() }
            }
        }
    }

    ConfirmDialog {
        id: conflictDialog
        parent: root
        anchors.fill: parent
        z: 210
        title: "Replace conflicting assignment?"
        message: editor.conflictMessage
        confirmLabel: "Replace"
        onConfirmed: editor.confirmConflict()
        onCanceled: editor.dismissConflict()
    }
    Connections {
        target: editor
        function onConflictChanged() {
            if (editor.conflictPending) conflictDialog.open()
            else conflictDialog.close()
        }
    }

    ConfirmDialog {
        id: resetProfileDialog
        parent: root
        anchors.fill: parent
        z: 210
        title: "Restore displayed bindings?"
        message: "Only the currently displayed device/profile overrides will be removed."
        confirmLabel: "Restore defaults"
        onConfirmed: editor.resetCurrentProfile()
    }
}
