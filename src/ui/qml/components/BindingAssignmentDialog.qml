import QtQuick
import QtQuick.Layouts
import GameHQ

// A draft editor for the complete assignment. Stored values are presented as
// fields; only the small Change/Record controls start live input capture.
FocusScope {
    id: root

    property var model: null   // BindingEditorModel

    visible: model && model.editorOpen
    opacity: visible ? 1 : 0
    Behavior on opacity { NumberAnimation { duration: Theme.durFast } }

    readonly property bool capturing: model && model.editorCaptureStep !== "idle"
    readonly property bool combination: model && model.editorTriggerKind === "combination"
    readonly property string noticeKind: model ? model.editorNoticeKind : "none"
    readonly property bool noticeDanger: noticeKind === "hard_conflict"
                                          || noticeKind === "invalid_pattern"
    readonly property bool noticeWarning: noticeKind === "unsupported_input"
                                           || noticeKind === "conversion_required"

    function gestureValue() {
        if (!model)
            return ""
        if (model.editorGestureKind === "tap")
            return "tap:" + model.editorTapCount
        return model.editorGestureKind + ":1"
    }

    function selectGesture(value) {
        const parts = value.split(":")
        const kind = parts[0]
        const taps = Number(parts[1])
        model.setEditorGesture(kind, taps, kind === "hold" ? model.editorHoldMs : 0)
    }

    onVisibleChanged: {
        if (!visible)
            return
        Qt.callLater(function() {
            const first = root.nextItemInFocusChain(true)
            if (first && first !== root)
                first.forceActiveFocus()
        })
    }

    Keys.onEscapePressed: function(event) {
        if (root.capturing)
            root.model.cancelTriggerCapture()
        else
            root.model.closeAssignmentEditor()
        event.accepted = true
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.scrim
        MouseArea {
            anchors.fill: parent
            // Clicking away discards only the draft; saved bindings stay put.
            onClicked: root.model.closeAssignmentEditor()
        }
    }

    Rectangle {
        id: dialogPanel
        anchors.centerIn: parent
        width: Math.min(parent.width - Theme.s16 * 2,
                        Theme.dialogWidth + Theme.s48 + Theme.s32)
        height: Math.min(parent.height - Theme.s16 * 2,
                         dialogContent.implicitHeight + Theme.s48)
        radius: Theme.radiusL
        color: Theme.surface
        border.width: root.capturing ? Theme.borderWidth + 1 : Theme.borderWidth
        border.color: root.capturing ? Theme.accent : Theme.stroke

        MouseArea { anchors.fill: parent }   // swallow clicks behind the panel

        Flickable {
            id: dialogViewport
            anchors.fill: parent
            anchors.margins: Theme.s24
            contentWidth: width
            contentHeight: dialogContent.implicitHeight
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            flickableDirection: Flickable.VerticalFlick

            ColumnLayout {
                id: dialogContent
                width: dialogViewport.width
                spacing: Theme.s16

                Text {
                    Layout.fillWidth: true
                    text: root.model ? root.model.editorActionLabel : ""
                    color: Theme.text
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontTitle
                    font.weight: Font.DemiBold
                    elide: Text.ElideRight
                }
                Text {
                    Layout.fillWidth: true
                    text: root.model
                          ? root.model.editorScopeLabel + " · Slot " + root.model.editorSlot
                          : ""
                    color: Theme.textMuted
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontCaption
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Theme.s8
                    visible: root.model && root.model.editorCombinationAvailable

                    Text {
                        text: "Pattern"
                        color: Theme.textMuted
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontCaption
                    }
                    BindingSegmentedControl {
                        Layout.fillWidth: true
                        currentValue: root.combination ? "combination" : "single"
                        options: [
                            { label: "Single button", value: "single" },
                            { label: "Combination", value: "combination" }
                        ]
                        onActivated: function(value) { root.model.setEditorTriggerKind(value) }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: captureModeText.implicitHeight + Theme.s12
                    visible: root.capturing
                    radius: Theme.radiusS
                    color: Theme.accentSoft
                    border.width: Theme.borderWidth
                    border.color: Theme.accent

                    Text {
                        id: captureModeText
                        anchors.centerIn: parent
                        text: "CONTROLLER CAPTURE ACTIVE · Dialog navigation is paused"
                        color: Theme.accent
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontCaption
                        font.letterSpacing: Theme.letterSpacingWide
                        horizontalAlignment: Text.AlignHCenter
                    }
                }

                // Single-button input: one value row with one compact action.
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Theme.s8
                    visible: !root.combination

                    Text {
                        text: "Input"
                        color: Theme.textMuted
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontCaption
                    }
                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: Theme.s48
                        radius: Theme.radiusS
                        color: root.capturing ? Theme.accentSoft : Theme.bg1
                        border.width: root.capturing ? Theme.borderWidth + 1 : Theme.borderWidth
                        border.color: root.capturing ? Theme.accent : Theme.stroke

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: Theme.s12
                            anchors.rightMargin: Theme.s8
                            spacing: Theme.s8

                            Text {
                                Layout.fillWidth: true
                                Layout.minimumWidth: 0
                                text: root.capturing
                                      ? "Listening… Press a controller button"
                                      : root.model && root.model.editorFirstControlLabel !== ""
                                        ? root.model.editorFirstControlLabel : "Not set"
                                color: root.capturing ? Theme.accent : Theme.text
                                font.family: Theme.fontFamily
                                font.pixelSize: Theme.fontBody
                                font.weight: Font.DemiBold
                                elide: Text.ElideRight
                            }
                            AccentButton {
                                quiet: true
                                label: root.capturing ? "Stop"
                                      : root.model && root.model.editorFirstControlLabel !== ""
                                        ? "Change" : "Record"
                                onClicked: {
                                    if (root.capturing) root.model.cancelTriggerCapture()
                                    else root.model.beginTriggerCapture(1)
                                }
                            }
                        }
                    }
                }

                // Combinations expose both ordered controls and make capture
                // progress explicit instead of flattening it into one label.
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Theme.s8
                    visible: root.combination

                    Text {
                        text: "First button"
                        color: Theme.textMuted
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontCaption
                    }
                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: Theme.s48
                        radius: Theme.radiusS
                        color: root.model && root.model.editorCaptureStep === "first"
                               ? Theme.accentSoft : Theme.bg1
                        border.width: root.model && root.model.editorCaptureStep === "first"
                                      ? Theme.borderWidth + 1 : Theme.borderWidth
                        border.color: root.model && root.model.editorCaptureStep === "first"
                                      ? Theme.accent : Theme.stroke

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: Theme.s12
                            anchors.rightMargin: Theme.s8
                            spacing: Theme.s8
                            Text {
                                Layout.fillWidth: true
                                Layout.minimumWidth: 0
                                text: root.model && root.model.editorCaptureStep === "first"
                                      ? "Listening… Hold the first button"
                                      : root.model && root.model.editorFirstControlLabel !== ""
                                        ? root.model.editorFirstControlLabel
                                          + (root.model.editorCaptureStep === "second" ? " detected" : "")
                                        : "Not set"
                                color: root.model && root.model.editorCaptureStep === "first"
                                       ? Theme.accent : Theme.text
                                font.family: Theme.fontFamily
                                font.pixelSize: Theme.fontBody
                                font.weight: Font.DemiBold
                                elide: Text.ElideRight
                            }
                            AccentButton {
                                visible: !root.capturing
                                quiet: true
                                label: root.model && root.model.editorFirstControlLabel !== ""
                                       ? "Change" : "Record"
                                onClicked: root.model.beginTriggerCapture(1)
                            }
                        }
                    }

                    Text {
                        text: "Second button"
                        color: Theme.textMuted
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontCaption
                    }
                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: Theme.s48
                        radius: Theme.radiusS
                        color: root.model && root.model.editorCaptureStep === "second"
                               ? Theme.accentSoft : Theme.bg1
                        border.width: root.model && root.model.editorCaptureStep === "second"
                                      ? Theme.borderWidth + 1 : Theme.borderWidth
                        border.color: root.model && root.model.editorCaptureStep === "second"
                                      ? Theme.accent : Theme.stroke

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: Theme.s12
                            anchors.rightMargin: Theme.s8
                            spacing: Theme.s8
                            Text {
                                Layout.fillWidth: true
                                Layout.minimumWidth: 0
                                text: root.model && root.model.editorCaptureStep === "second"
                                      ? "Listening… Press the second button"
                                      : root.model && root.model.editorSecondControlLabel !== ""
                                        ? root.model.editorSecondControlLabel
                                        : root.model && root.model.editorFirstControlLabel === ""
                                          ? "Waiting for first button" : "Not set"
                                color: root.model && root.model.editorCaptureStep === "second"
                                       ? Theme.accent : Theme.text
                                font.family: Theme.fontFamily
                                font.pixelSize: Theme.fontBody
                                font.weight: Font.DemiBold
                                elide: Text.ElideRight
                            }
                            AccentButton {
                                visible: !root.capturing
                                quiet: true
                                enabled: root.model && root.model.editorFirstControlLabel !== ""
                                label: root.model && root.model.editorSecondControlLabel !== ""
                                       ? "Change" : "Record"
                                onClicked: root.model.beginTriggerCapture(2)
                            }
                        }
                    }
                    Text {
                        Layout.fillWidth: true
                        visible: !root.capturing && root.model
                                 && root.model.editorTriggerHint !== ""
                        text: root.model ? root.model.editorTriggerHint : ""
                        color: Theme.textMuted
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontCaption
                        wrapMode: Text.WordWrap
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Theme.s8
                    visible: root.model && !root.model.editorGestureLocked

                    Text {
                        text: "Gesture"
                        color: Theme.textMuted
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontCaption
                    }
                    BindingSegmentedControl {
                        Layout.fillWidth: true
                        currentValue: root.gestureValue()
                        options: [
                            { label: "Press", value: "press:1" },
                            { label: "Tap", value: "tap:1" },
                            { label: "Double tap", value: "tap:2" },
                            { label: "Triple tap", value: "tap:3" },
                            { label: "Hold", value: "hold:1" }
                        ]
                        onActivated: function(value) { root.selectGesture(value) }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    visible: root.model && root.model.editorGestureLocked
                    spacing: Theme.s8
                    Text {
                        text: "Gesture"
                        color: Theme.textMuted
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontCaption
                    }
                    Rectangle {
                        implicitWidth: lockedGestureText.implicitWidth + Theme.s12
                        implicitHeight: lockedGestureText.implicitHeight + Theme.s4
                        radius: Theme.radiusPill
                        color: Theme.bg1
                        border.width: Theme.borderWidth
                        border.color: Theme.stroke
                        Text {
                            id: lockedGestureText
                            anchors.centerIn: parent
                            text: "Press · fixed for combinations"
                            color: Theme.textMuted
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.fontCaption
                        }
                    }
                    Item { Layout.fillWidth: true }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Theme.s8
                    visible: root.model && root.model.editorGestureKind === "hold"

                    Text {
                        text: "Hold duration"
                        color: Theme.textMuted
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontCaption
                    }
                    BindingSegmentedControl {
                        Layout.fillWidth: true
                        currentValue: root.model ? String(root.model.editorHoldMs) : "0"
                        options: [
                            { label: "Default", value: "0" },
                            { label: "1.0 s", value: "1000" },
                            { label: "1.5 s", value: "1500" },
                            { label: "2.0 s", value: "2000" },
                            { label: "3.0 s", value: "3000" }
                        ]
                        onActivated: function(value) {
                            root.model.setEditorGesture("hold", 1, Number(value))
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: noticeRow.implicitHeight + Theme.s16 * 2
                    visible: root.model && root.model.editorNotice !== ""
                    radius: Theme.radiusS
                    color: root.noticeDanger ? Theme.dangerSoft
                         : root.noticeWarning ? Theme.warningSoft
                                                                  : Theme.surfaceAlt
                    border.width: Theme.borderWidth
                    border.color: root.noticeDanger ? Theme.danger
                                : root.noticeKind === "unsupported_input" ? Theme.warning
                                                                         : Theme.stroke

                    RowLayout {
                        id: noticeRow
                        x: Theme.s16
                        y: Theme.s16
                        width: parent.width - Theme.s16 * 2
                        spacing: Theme.s12

                        Text {
                            text: root.noticeDanger ? "!" : root.noticeWarning ? "⚠" : "i"
                            color: root.noticeDanger ? Theme.danger
                                 : root.noticeWarning ? Theme.warning : Theme.textMuted
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.fontH3
                            font.weight: Font.DemiBold
                            Layout.alignment: Qt.AlignTop
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: Theme.s4
                            Text {
                                Layout.fillWidth: true
                                text: root.noticeDanger ? "Assignment needs attention"
                                      : root.noticeKind === "unsupported_input"
                                        ? "Button not verified this session"
                                      : root.noticeKind === "conversion_required"
                                        ? "Compatibility change required" : "Assignment note"
                                color: root.noticeDanger ? Theme.danger
                                     : root.noticeWarning ? Theme.warning : Theme.text
                                font.family: Theme.fontFamily
                                font.pixelSize: Theme.fontBody
                                font.weight: Font.DemiBold
                                wrapMode: Text.WordWrap
                            }
                            Text {
                                Layout.fillWidth: true
                                text: root.model ? root.model.editorNotice : ""
                                color: Theme.textMuted
                                font.family: Theme.fontFamily
                                font.pixelSize: Theme.fontCaption
                                wrapMode: Text.WordWrap
                            }
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.s12
                    Item { Layout.fillWidth: true }
                    AccentButton {
                        label: "Cancel"
                        onClicked: root.model.closeAssignmentEditor()
                    }
                    AccentButton {
                        primary: true
                        label: "Save"
                        enabled: root.model && root.model.editorCanSave
                        onClicked: root.model.saveAssignment()
                    }
                }
            }
        }
    }
}
