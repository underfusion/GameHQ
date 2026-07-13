import QtQuick
import GameHQ

Rectangle {
    id: root

    property var categories: []
    property int sidebarIndex: 0

    signal entrySelected(int index)

    width: 260
    radius: Theme.radiusL
    color: Theme.panelTint
    border.width: 1
    border.color: Theme.stroke

    MouseArea { anchors.fill: parent }

    Column {
        anchors.fill: parent
        anchors.margins: Theme.s12
        spacing: Theme.s4

        Repeater {
            model: root.categories
            delegate: SidebarItem {
                width: parent.width
                label: modelData.label
                glyph: modelData.glyph
                active: root.sidebarIndex === index
                onClicked: root.entrySelected(index)
            }
        }

        Text {
            text: "GAMES"
            color: Theme.textFaint
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontCaption
            font.letterSpacing: 1
            topPadding: Theme.s16
            leftPadding: Theme.s8
        }

        Repeater {
            model: app.games
            delegate: SidebarItem {
                width: parent.width
                label: modelData.name
                iconSource: modelData.iconPath ? ("file:///" + modelData.iconPath.replace(/\\/g, "/")) : ""
                active: root.sidebarIndex === (root.categories.length + index)
                        && !(app.currentGameAvailable && app.currentGameId === modelData.id)
                onClicked: root.entrySelected(root.categories.length + index)
            }
        }
    }

    Item {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: Theme.s12
        height: Math.max(brandIcon.height, brandLabel.implicitHeight)

        Image {
            id: brandIcon
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            source: "qrc:/icons/gamehq.svg"
            width: Theme.fontH3
            height: Theme.fontH3
            sourceSize.width: Theme.fontH3
            sourceSize.height: Theme.fontH3
        }

        Text {
            id: brandLabel
            anchors.left: brandIcon.right
            anchors.leftMargin: Theme.s8
            anchors.verticalCenter: parent.verticalCenter
            text: Brand.name
            color: Theme.textMuted
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontH3
            font.weight: Font.Light
        }
    }
}
