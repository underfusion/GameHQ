import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic as QC
import GameHQ

// Help page — keyboard shortcuts, controller bindings, and quick feature reference.
Item {
    id: root

    function resetScroll() {
        flick.contentY = 0
    }

    function scrollBy(direction) {
        const maximum = Math.max(0, flick.contentHeight - flick.height)
        flick.contentY = Math.max(0, Math.min(maximum,
            flick.contentY + direction * Math.max(80, flick.height * 0.28)))
    }

    Flickable {
        id: flick
        anchors.fill: parent
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        contentWidth: width
        contentHeight: helpColumn.implicitHeight + Theme.s16
        flickableDirection: Flickable.VerticalFlick

        QC.ScrollBar.vertical: QC.ScrollBar {
            policy: QC.ScrollBar.AlwaysOn
        }

        ColumnLayout {
            id: helpColumn
            width: Math.max(0, flick.width - Theme.s16)
            spacing: Theme.s24

    // ── Keyboard shortcuts ──
    ColumnLayout {
        spacing: Theme.s8
        Text {
            text: "KEYBOARD SHORTCUTS"
            color: Theme.textFaint
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontCaption
            font.letterSpacing: Theme.letterSpacingWide
        }
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: keysCol.implicitHeight + Theme.s16 * 2
            radius: Theme.radiusM
            color: Theme.surface
            border.width: 1
            border.color: Theme.stroke

            ColumnLayout {
                id: keysCol
                anchors.fill: parent
                anchors.margins: Theme.s16
                spacing: Theme.s8

                Repeater {
                    model: [
                        { binding: "Alt+Shift+G", act: "Open / close overlay" },
                        { binding: "Ctrl+Shift+S", act: "Take screenshot" },
                        { binding: "Ctrl+Shift+E", act: "Save last N seconds as clip" },
                        { binding: "Enter",        act: "Open selected capture" },
                        { binding: "Select mode",   act: "Enter / Space toggles, Ctrl+A selects all, Delete removes selected" },
                        { binding: "F",            act: "Favourite / unfavourite selected" },
                        { binding: "E",            act: "Show selected in Explorer" },
                        { binding: "W / A / S / D",act: "Navigate gallery grid" }
                    ]
                    delegate: RowLayout {
                        Layout.fillWidth: true
                        Rectangle {
                            Layout.minimumWidth: 160
                            implicitWidth: bindingLabel.implicitWidth + Theme.s8 * 2
                            implicitHeight: 28
                            radius: Theme.radiusS
                            color: Theme.surfaceAlt
                            Text {
                                id: bindingLabel
                                anchors.centerIn: parent
                                text: modelData.binding
                                color: Theme.accent
                                font.family: Theme.fontFamily
                                font.pixelSize: Theme.fontBody
                                font.weight: Font.DemiBold
                            }
                        }
                        Text {
                            text: modelData.act
                            color: Theme.text
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.fontBody
                            Layout.fillWidth: true
                        }
                    }
                }
            }
        }
    }

    // ── Controller (DualSense / gamepad) ──
    ColumnLayout {
        spacing: Theme.s8
        Layout.fillWidth: true
        Text {
            text: "DUALSENSE / GAMEPAD"
            color: Theme.textFaint
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontCaption
            font.letterSpacing: Theme.letterSpacingWide
        }
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: ctrlCol.implicitHeight + Theme.s16 * 2
            radius: Theme.radiusM
            color: Theme.surface
            border.width: 1
            border.color: Theme.stroke

            ColumnLayout {
                id: ctrlCol
                anchors.fill: parent
                anchors.margins: Theme.s16
                spacing: Theme.s8

                Repeater {
                    model: [
                        { binding: "Share (tap)",      act: "Take screenshot" },
                        { binding: "Share (hold 1s)",  act: "Save replay clip (last N seconds)" },
                        { binding: "PS button",        act: "Open / close overlay" },
                        { binding: "L1 / R1",          act: "Switch panel: sidebar ↔ grid (app) / flip captures (overlay)" },
                        { binding: "D-pad / Left Stick",act: "Navigate gallery grid" },
                        { binding: "Cross",            act: "Open selected capture / confirm" },
                        { binding: "Circle",           act: "Back / close overlay" },
                        { binding: "Square",           act: "Action menu (Show in folder / Delete)" },
                        { binding: "Triangle",         act: "Favourite / unfavourite selected" },
                        { binding: "Select mode",      act: "Cross toggles, Triangle selects all, Square deletes, Circle exits" }
                    ]
                    delegate: RowLayout {
                        Layout.fillWidth: true
                        Rectangle {
                            Layout.minimumWidth: 130
                            implicitWidth: ctrlLabel.implicitWidth + Theme.s8 * 2
                            implicitHeight: 28
                            radius: Theme.radiusS
                            color: Theme.surfaceAlt
                            Text {
                                id: ctrlLabel
                                anchors.centerIn: parent
                                text: modelData.binding
                                color: Theme.accent
                                font.family: Theme.fontFamily
                                font.pixelSize: Theme.fontBody
                                font.weight: Font.DemiBold
                            }
                        }
                        Text {
                            text: modelData.act
                            color: Theme.text
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.fontBody
                            Layout.fillWidth: true
                        }
                    }
                }
            }
        }
    }

    // ── Features quick reference ──
    ColumnLayout {
        spacing: Theme.s8
        Layout.fillWidth: true
        Text {
            text: "FEATURES"
            color: Theme.textFaint
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontCaption
            font.letterSpacing: Theme.letterSpacingWide
        }
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: featCol.implicitHeight + Theme.s16 * 2
            radius: Theme.radiusM
            color: Theme.surface
            border.width: 1
            border.color: Theme.stroke

            ColumnLayout {
                id: featCol
                anchors.fill: parent
                anchors.margins: Theme.s16
                spacing: Theme.s8

                Repeater {
                    model: [
                        { heading: "Replay buffer",  desc: "Always-on auto-armed. Records in the background while a game is in focus. Hold Share (or Ctrl+Shift+E) to save the last few seconds as a clip. Turn always-on recording on or off in Settings → Replay." },
                        { heading: "Screenshots",    desc: "GDI grab of the active game window. PNG saved to your captures folder with instant shutter feedback." },
                        { heading: "Gallery",        desc: "All captures in one grid — filter by category or game. Grid navigation works with keyboard, mouse, and controller." },
                        { heading: "Overlay",        desc: "Transparent fullscreen HUD. View and manage captures from inside a game without alt-tabbing. Includes its own gallery grid, lightbox, and toast notifications." },
                        { heading: "Lightbox",       desc: "Full-screen viewer for screenshots and videos. Opens from both the main window and the overlay." },
                        { heading: "Watched folders",desc: "Add any folder (Game Bar, Steam, NVIDIA ShadowPlay) — " + Brand.name + " scans it for new captures automatically." }
                    ]
                    delegate: ColumnLayout {
                        Layout.fillWidth: true
                        spacing: Theme.s4
                        Text {
                            text: modelData.heading
                            color: Theme.text
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.fontH3
                            font.weight: Font.DemiBold
                        }
                        Text {
                            text: modelData.desc
                            color: Theme.textMuted
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.fontBody
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                        }
                        Item { Layout.preferredHeight: Theme.s4 } // spacer between entries
                    }
                }
            }
        }
    }

        }
    }
}
