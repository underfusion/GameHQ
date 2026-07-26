import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic as QC
import GameHQ

Item {
    id: root

    property string pageTitle: ""
    property string pageDescription: ""
    property Component pageAction
    // Zero means fill the available page area. Individual pages may still set
    // a positive cap when a deliberately narrow reading column is useful.
    property int maxContentWidth: 0
    default property alias contentData: contentColumn.data

    function containsItem(item) {
        for (let probe = item; probe; probe = probe.parent)
            if (probe === root)
                return true
        return false
    }

    function revealFocusedItem() {
        const item = Window.window ? Window.window.activeFocusItem : null
        if (!item || !containsItem(item))
            return
        const point = item.mapToItem(pageContainer, 0, 0)
        const top = Math.max(0, point.y - Theme.s16)
        const bottom = point.y + item.height + Theme.s16
        if (top < flick.contentY)
            flick.contentY = top
        else if (bottom > flick.contentY + flick.height)
            flick.contentY = Math.min(flick.contentHeight - flick.height,
                                      bottom - flick.height)
    }

    Connections {
        target: Window.window
        function onActiveFocusItemChanged() { Qt.callLater(root.revealFocusedItem) }
    }

    Flickable {
        id: flick
        anchors.fill: parent
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        contentWidth: width
        contentHeight: pageContainer.implicitHeight + Theme.s24
        flickableDirection: Flickable.VerticalFlick

        QC.ScrollBar.vertical: QC.ScrollBar {
            policy: flick.contentHeight > flick.height ? QC.ScrollBar.AsNeeded
                                                       : QC.ScrollBar.AlwaysOff
        }

        MouseArea {
            width: flick.contentWidth
            height: Math.max(flick.height, flick.contentHeight)
            z: -1
            onClicked: root.forceActiveFocus()
        }

        ColumnLayout {
            id: pageContainer
            x: Math.max(0, (flick.width - width) / 2)
            width: Math.max(0, root.maxContentWidth > 0
                                  ? Math.min(root.maxContentWidth, flick.width - Theme.s16 * 2)
                                  : flick.width - Theme.s16 * 2)
            spacing: Theme.s24

            SettingsPageHeader {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                visible: root.pageTitle.length > 0 || root.pageDescription.length > 0
                         || root.pageAction !== null
                title: root.pageTitle
                description: root.pageDescription
                action: root.pageAction
            }

            ColumnLayout {
                id: contentColumn
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                spacing: Theme.s16
            }
        }
    }
}
