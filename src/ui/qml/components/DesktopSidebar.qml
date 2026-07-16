import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import GameHQ

Rectangle {
    id: root

    property var categories: []
    property bool settingsOpen: false
    property bool helpOpen: false
    property bool sidebarFocused: false
    property int sidebarHoverIndex: 0

    signal settingsRequested()
    signal helpRequested()
    signal pageClosed()

    Layout.preferredWidth: 220
    Layout.fillHeight: true
    radius: Theme.radiusL
    color: Theme.surface
    border.width: 1
    border.color: Theme.stroke

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.s12
        spacing: Theme.s4

        RowLayout {
            Layout.margins: Theme.s8
            spacing: Theme.s8

            Image {
                source: "qrc:/icons/gamehq.svg"
                Layout.preferredWidth: Theme.fontTitle
                Layout.preferredHeight: Theme.fontTitle
                sourceSize.width: Theme.fontTitle
                sourceSize.height: Theme.fontTitle
            }

            Text {
                text: Brand.name
                color: Theme.text
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontTitle
                font.weight: Font.DemiBold
            }
        }

        Repeater {
            model: root.categories
            delegate: SidebarItem {
                Layout.fillWidth: true
                label: modelData.label
                glyph: modelData.glyph
                active: !root.settingsOpen && !root.helpOpen
                        && ((modelData.key === "game" && app.currentGameAvailable
                                && app.gameId === app.currentGameId && app.category !== "favorites")
                            || (modelData.key === "game_favorites" && app.currentGameAvailable
                                && app.gameId === app.currentGameId && app.category === "favorites")
                            || (modelData.key !== "game" && app.category === modelData.key && app.gameId < 0))
                sidebarHovered: root.sidebarFocused && root.sidebarHoverIndex === index
                onClicked: {
                    root.pageClosed()
                    if (modelData.key === "game")
                        app.setGame(app.currentGameId)
                    else if (modelData.key === "game_favorites")
                        app.setGameCategory("favorites", app.currentGameId)
                    else
                        app.setCategory(modelData.key)
                }
            }
        }

        Text {
            text: "GAMES"
            color: Theme.textFaint
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontCaption
            font.letterSpacing: Theme.letterSpacingWide
            Layout.margins: Theme.s8
            Layout.topMargin: Theme.s16
        }

        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: Theme.s4
            model: app.games
            delegate: SidebarItem {
                width: ListView.view.width
                label: modelData.name
                iconSource: modelData.iconPath ? ("file:///" + modelData.iconPath.replace(/\\/g, "/")) : ""
                active: !root.settingsOpen && !root.helpOpen && app.gameId === modelData.id
                        && !(app.currentGameAvailable && app.currentGameId === modelData.id)
                sidebarHovered: root.sidebarFocused && root.sidebarHoverIndex === root.categories.length + index
                onClicked: {
                    root.pageClosed()
                    app.setGame(modelData.id)
                }
            }
        }

        SidebarItem {
            Layout.fillWidth: true
            label: "Settings"
            glyph: "\u2699"
            active: root.settingsOpen
            sidebarHovered: root.sidebarFocused && root.sidebarHoverIndex === root.categories.length + app.games.length
            onClicked: root.settingsRequested()
        }

        SidebarItem {
            Layout.fillWidth: true
            label: "Help"
            glyph: "?"
            active: root.helpOpen
            sidebarHovered: root.sidebarFocused && root.sidebarHoverIndex === root.categories.length + app.games.length + 1
            onClicked: root.helpRequested()
        }

        Text {
            text: "v" + app.version
            color: Theme.textFaint
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontCaption
            Layout.alignment: Qt.AlignHCenter
            Layout.bottomMargin: Theme.s4
        }
    }
}
