import QtQuick
import QtQuick.Layouts
import GameHQ
import "../components"

SettingsPage {
    id: root
    pageTitle: "Input"
    pageDescription: "Configure controller, keyboard, and mouse shortcuts without changing navigation behavior."
    readonly property var editor: input.bindingEditor

    SettingsSection {
        eyebrow: "Devices"
        title: "Input devices"
        description: "Choose a device type, then select either assignment slot to capture a new input."
        SettingsRow {
            label: "Device type"
            description: editor.deviceGroup === "controller" ? input.controllerStatus
                       : editor.deviceGroup === "keyboard" ? "Focused shortcuts and global key combinations"
                                                           : "Middle, Back, and Forward mouse buttons"
            SettingsSegmentedControl {
                currentValue: editor.deviceGroup
                options: [
                    { label: "Controller", value: "controller" },
                    { label: "Keyboard", value: "keyboard" },
                    { label: "Mouse", value: "mouse" }
                ]
                onActivated: function(value) { editor.deviceGroup = value }
            }
        }
        SettingsRow {
            visible: editor.deviceGroup === "controller"
            label: "Controller profile"
            description: editor.controllerSpecific
                         ? "Changes apply only to " + editor.controllerName + "."
                         : "Position-based assignments work across PlayStation, Xbox, Nintendo, and generic pads."
            SettingsSegmentedControl {
                currentValue: editor.controllerSpecific ? "specific" : "shared"
                options: editor.controllerSpecificAvailable
                    ? [
                        { label: "All controllers", value: "shared" },
                        { label: editor.controllerName.length > 0 ? editor.controllerName : "This controller", value: "specific" }
                      ]
                    : [{ label: "All controllers", value: "shared" }]
                onActivated: function(value) { editor.controllerSpecific = value === "specific" }
            }
        }
    }

    SettingsSection {
        visible: input.controllerWarning.length > 0
        eyebrow: "Attention"
        title: "Controller hidden"
        description: input.controllerWarning
        variant: "warning"
        headerAction: Component {
            AccentButton {
                visible: input.controllerFixAvailable
                label: "Fix automatically"
                primary: true
                onClicked: input.fixHiddenController()
            }
        }
    }

    // Non-blocking result of the last assignment. Deliberately its own section
    // on its own property: input.controllerWarning above is reserved for
    // HidHide/cloaked-pad state, and a routine binding notice must never
    // overwrite the one warning the user cannot diagnose on their own.
    // HardConflict never lands here — it takes over the modal dialog instead.
    SettingsSection {
        visible: editor.relationNotice.length > 0
                 && editor.relationKind !== "none"
                 && editor.relationKind !== "hard_conflict"
        // Three different failures, three different words. "Button not
        // reported" is only ever the controller/backend case — a chord Windows
        // owns and a failed write are separate kinds with their own copy.
        eyebrow: editor.relationKind === "context_override" ? "Context"
                 : editor.relationKind === "conversion_required" ? "Compatibility"
                 : editor.relationKind === "unsupported_input" ? "Not available"
                 : editor.relationKind === "hotkey_unavailable" ? "In use"
                 : editor.relationKind === "persistence_error" ? "Not saved"
                 : editor.relationKind === "redundant" ? "Duplicate"
                                                       : "Shared button"
        title: editor.relationKind === "context_override" ? "This button changes meaning"
               : editor.relationKind === "conversion_required" ? "Assignment conversion required"
               : editor.relationKind === "unsupported_input" ? "Button not reported"
               : editor.relationKind === "hotkey_unavailable" ? "Shortcut already taken"
               : editor.relationKind === "persistence_error" ? "Could not save this binding"
               : editor.relationKind === "redundant" ? "Already assigned"
                                                     : "One button, several gestures"
        description: editor.relationNotice
        // Context overrides and the three failure kinds are worth a second
        // look; shared gestures and duplicates are informational, so they stay
        // quiet.
        variant: editor.relationKind === "context_override"
                 || editor.relationKind === "conversion_required"
                 || editor.relationKind === "unsupported_input"
                 || editor.relationKind === "hotkey_unavailable"
                 || editor.relationKind === "persistence_error" ? "warning" : "status"
        headerAction: Component {
            AccentButton {
                label: "Dismiss"
                quiet: true
                onClicked: editor.dismissRelationNotice()
            }
        }
    }

    SettingsSection {
        eyebrow: "Profile"
        title: "Test and restore"
        variant: "compact"
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
            AccentButton { label: "Restore defaults"; quiet: true; onClicked: resetProfileDialog.open() }
        }
        SettingsRow {
            visible: editor.legacyCopyAvailable
            label: "Adopt per-slot bindings"
            description: "Copy bindings saved for any controller in this slot to this specific controller. The originals are kept."
            AccentButton {
                label: "Copy to this controller"
                quiet: true
                onClicked: editor.copyLegacyOverridesToController()
            }
        }
        SettingsRow {
            label: "Identify a controller button"
            description: input.probeRunning || input.probeStatus.length > 0
                         ? input.probeStatus
                         : "Records the next 3 seconds of raw button changes — including buttons GameHQ does not recognize — into the diagnostics you can copy from Advanced."
            AccentButton {
                label: "Start 3-second probe"
                quiet: true
                enabled: !input.probeRunning
                onClicked: input.startButtonProbe()
            }
        }
    }

    SettingsSection {
        eyebrow: "Gestures"
        title: "Gesture timing"
        description: "How long GameHQ waits before it decides what a button press meant."
        SettingsRow {
            label: "Hold time"
            description: "How long a button must be held for a hold action. A completed hold consumes the tap."
            SettingsCombo {
                configKey: "input.default_hold_ms"; defaultValue: 2000
                options: [
                    { label: "1.0 seconds", value: 1000 }, { label: "1.5 seconds", value: 1500 },
                    { label: "2.0 seconds", value: 2000 }, { label: "3.0 seconds", value: 3000 }
                ]
            }
        }
        SettingsRow {
            label: "Multi-tap interval"
            description: "How long a single tap waits when the same button also has a double or triple tap."
            SettingsCombo {
                configKey: "input.multi_tap_interval_ms"; defaultValue: 300
                options: [
                    { label: "200 ms (fast)", value: 200 }, { label: "300 ms", value: 300 },
                    { label: "400 ms", value: 400 }, { label: "500 ms (relaxed)", value: 500 }
                ]
            }
        }
        SettingsRow {
            label: "Combination window"
            description: "How long the first button of a combination waits for the second one."
            SettingsCombo {
                configKey: "input.chord_window_ms"; defaultValue: 300
                options: [
                    { label: "200 ms (fast)", value: 200 }, { label: "300 ms", value: 300 },
                    { label: "400 ms", value: 400 }, { label: "500 ms (relaxed)", value: 500 }
                ]
            }
        }
    }

    SettingsSection {
        id: assignmentsSection
        eyebrow: "Bindings"
        title: "Assignments"
        description: "Primary and secondary slots are independent. Contexts can reuse the same input safely."

        Repeater {
            model: editor.rows
            delegate: Rectangle {
                id: actionCard
                property bool modified: Boolean(modelData.modified)

                Layout.fillWidth: true
                implicitHeight: actionLayout.implicitHeight + Theme.s32
                radius: Theme.radiusM
                color: modified
                       ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.07)
                       : Theme.bg1
                border.width: Theme.borderWidth
                border.color: Theme.stroke

                Behavior on color { ColorAnimation { duration: Theme.durFast } }
                Rectangle {
                    visible: actionCard.modified
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    width: 3
                    radius: actionCard.radius
                    color: Theme.accent
                }

                ColumnLayout {
                    id: actionLayout
                    x: Theme.s16
                    y: Theme.s16
                    width: parent.width - Theme.s32
                    spacing: Theme.s12

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.s12

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: Theme.s4
                            Text {
                                text: modelData.label
                                color: Theme.text
                                font.family: Theme.fontFamily
                                font.pixelSize: Theme.fontH3
                                font.weight: Font.DemiBold
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
                        AccentButton {
                            visible: modelData.bindable
                            label: "Restore defaults"
                            quiet: true
                            enabled: modified
                            labelColor: modified ? Theme.accent : Theme.textMuted
                            quietIdleBorderColor: modified ? Theme.accent : Theme.borderLight
                            onClicked: editor.resetAction(modelData.actionId)
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: assignmentLayout.implicitHeight + Theme.s24
                        radius: Theme.radiusM
                        color: Theme.surfaceAlt
                        border.width: Theme.borderWidth
                        border.color: Theme.stroke

                        ColumnLayout {
                            id: assignmentLayout
                            x: Theme.s12
                            y: Theme.s12
                            width: parent.width - Theme.s24
                            spacing: Theme.s8

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: Theme.s12
                                Text {
                                    text: "INPUT ASSIGNMENTS"
                                    color: Theme.textFaint
                                    font.family: Theme.fontFamily
                                    font.pixelSize: Theme.fontCaption
                                    font.letterSpacing: Theme.letterSpacingWide
                                }
                                Text {
                                    Layout.fillWidth: true
                                    text: "Both slots can be active. Select one to edit."
                                    color: Theme.textMuted
                                    font.family: Theme.fontFamily
                                    font.pixelSize: Theme.fontCaption
                                    horizontalAlignment: Text.AlignRight
                                    wrapMode: Text.WordWrap
                                }
                            }

                            GridLayout {
                                id: slotGrid
                                Layout.fillWidth: true
                                columns: width < 640 ? 1 : 2
                                columnSpacing: Theme.s8
                                rowSpacing: Theme.s8

                                BindingCard {
                                    Layout.fillWidth: true
                                    slotLabel: "Primary"
                                    assigned: modelData.primaryAssigned
                                    triggerLabel: modelData.primaryTrigger
                                    badgeLabel: modelData.bindable ? modelData.primaryGesture : "Fixed"
                                    editable: modelData.bindable
                                    changeState: modelData.primaryChangeState
                                    statusLabel: modelData.primaryStatusLabel
                                    onEditRequested: editor.openAssignmentEditor(modelData.actionId, 1)
                                    onClearRequested: editor.clearBinding(modelData.actionId, 1)
                                    onResetRequested: editor.resetBinding(modelData.actionId, 1)
                                }
                                BindingCard {
                                    Layout.fillWidth: true
                                    slotLabel: "Secondary"
                                    assigned: modelData.secondaryAssigned
                                    triggerLabel: modelData.secondaryTrigger
                                    badgeLabel: modelData.bindable ? modelData.secondaryGesture : "Fixed"
                                    editable: modelData.bindable
                                    changeState: modelData.secondaryChangeState
                                    statusLabel: modelData.secondaryStatusLabel
                                    onEditRequested: editor.openAssignmentEditor(modelData.actionId, 2)
                                    onClearRequested: editor.clearBinding(modelData.actionId, 2)
                                    onResetRequested: editor.resetBinding(modelData.actionId, 2)
                                }
                            }
                        }
                    }
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

    BindingAssignmentDialog {
        id: assignmentDialog
        parent: root
        anchors.fill: parent
        z: 205
        model: editor
    }

    BindingConflictDialog {
        id: conflictDialog
        parent: root
        anchors.fill: parent
        z: 210
        message: editor.conflictMessage
        onReplaced: editor.confirmConflict()
        // Re-arms capture on the same action and slot, so the user can pick a
        // different button without hunting for the row again.
        onRetried: editor.retryConflictCapture()
        onCanceled: editor.dismissConflict()
    }
    Connections {
        target: editor
        function onConflictChanged() {
            if (editor.conflictPending) conflictDialog.open()
            else conflictDialog.close()
        }
    }

    BindingCompatibilityDialog {
        id: compatibilityDialog
        parent: root
        anchors.fill: parent
        z: 211
        message: editor.compatibilityMessage
        onConverted: editor.confirmCompatibility()
        onRetried: editor.retryCompatibilityCapture()
        onCanceled: editor.dismissCompatibility()
    }
    Connections {
        target: editor
        function onCompatibilityChanged() {
            if (editor.compatibilityPending) compatibilityDialog.open()
            else compatibilityDialog.close()
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
