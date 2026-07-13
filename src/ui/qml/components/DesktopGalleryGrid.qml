import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import GameHQ

Item {
    id: root
    Layout.fillWidth: true
    Layout.fillHeight: true

    property var host
    property var deleteDialog
    property var bulkDeleteDialog
    property alias currentIndex: galleryGrid.currentIndex
    property alias count: galleryGrid.count
    property alias cellWidth: galleryGrid.cellWidth
    property alias _navLockUntil: galleryGrid._navLockUntil

    signal captureActivated(int index)
    signal deleteRequested(int index, string gameName, string dateText)
    signal addFolderRequested()

    function forceActiveFocus() { galleryGrid.forceActiveFocus() }
    function moveCurrentIndexLeft() { galleryGrid.moveCurrentIndexLeft() }
    function moveCurrentIndexRight() { galleryGrid.moveCurrentIndexRight() }
    function moveCurrentIndexUp() { galleryGrid.moveCurrentIndexUp() }
    function moveCurrentIndexDown() { galleryGrid.moveCurrentIndexDown() }

    GridView {
        id: galleryGrid
        anchors.fill: parent
        anchors.leftMargin: -Theme.s8
        anchors.rightMargin: Theme.s12
        clip: true
        model: app.gallery

        property point _lastHoverPos: Qt.point(-9999, -9999)
        property real _navLockUntil: 0

        HoverHandler { id: gridHover }

        cellWidth: galleryGrid.width > 0
            ? galleryGrid.width / root.host.gridColumns(galleryGrid.width)
            : root.host.zoomLevel
        cellHeight: galleryGrid.cellWidth * 9 / 16 + Theme.s32
        focus: true
        keyNavigationEnabled: false
        onCurrentIndexChanged: sounds.play("nav_tick")

        delegate: Item {
            width: galleryGrid.cellWidth
            height: galleryGrid.cellHeight

            HoverHandler {
                onHoveredChanged: {
                    if (!hovered) return
                    if (Date.now() < galleryGrid._navLockUntil) return
                    const p = gridHover.point.position
                    const px = Math.round(p.x), py = Math.round(p.y)
                    if (px !== galleryGrid._lastHoverPos.x || py !== galleryGrid._lastHoverPos.y) {
                        galleryGrid._lastHoverPos = Qt.point(px, py)
                        galleryGrid.currentIndex = index
                    }
                }
            }

            CaptureTile {
                anchors.fill: parent
                anchors.margins: Theme.s8
                thumbnail: model.thumbnail
                captureType: model.captureType
                gameName: model.gameName
                dateText: model.dateText
                favorite: model.favorite
                selected: galleryGrid.currentIndex === index
                bulkMode: root.host.bulkMode
                checked: root.host.bulkIsChecked(model.filePath)
                onActivated: {
                    galleryGrid.currentIndex = index
                    galleryGrid.forceActiveFocus()
                    root.captureActivated(index)
                }
                onCheckToggled: function(extendRange) {
                    galleryGrid.currentIndex = index
                    galleryGrid.forceActiveFocus()
                    root.host.bulkToggle(index, extendRange)
                }
                onOpenRequested: {
                    sounds.play("confirm")
                    app.openCapture(index)
                }
                onRequestOpenFolder: app.showInFolder(index)
                onToggleFavoriteRequested: {
                    sounds.play("favorite")
                    app.toggleFavorite(index)
                }
                onRequestDelete: root.deleteRequested(index, model.gameName, model.dateText)
            }
        }

        Keys.onPressed: (event) => {
            root.host.usingGamepad = false
            if (input.handleKeyPressed(event.key, event.modifiers,
                                       event.isAutoRepeat)) {
                event.accepted = true
                return
            }

            if (root.host.bulkMode && !root.bulkDeleteDialog.visible) {
                if (event.key === Qt.Key_Space) {
                    const rec = app.gallery.get(currentIndex)
                    if (rec.filePath)
                        root.host.bulkToggle(currentIndex,
                                             !!(event.modifiers & Qt.ShiftModifier))
                    event.accepted = true
                    return
                } else if (event.key === Qt.Key_Delete || event.key === Qt.Key_Backspace) {
                    root.host.bulkAskDelete()
                    event.accepted = true
                    return
                } else if (event.key === Qt.Key_A && (event.modifiers & Qt.ControlModifier)) {
                    root.host.bulkSelectAll()
                    event.accepted = true
                    return
                }
            }

            if (!root.host.bulkMode && event.key === Qt.Key_E) {
                app.showInFolder(currentIndex)
                event.accepted = true
            }
        }

        Keys.onReleased: (event) => {
            event.accepted = input.handleKeyReleased(event.key, event.modifiers)
        }

        DesktopEmptyState {
            visible: galleryGrid.count === 0
            onAddFolderRequested: root.addFolderRequested()
        }
    }

    ScrollBar {
        id: gridScroll
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        orientation: Qt.Vertical
        policy: ScrollBar.AlwaysOn
        size: galleryGrid.visibleArea.heightRatio
        Binding on position {
            value: galleryGrid.visibleArea.yPosition
            when: !gridScroll.pressed
            restoreMode: Binding.RestoreBinding
        }
        onPositionChanged: {
            if (gridScroll.pressed)
                galleryGrid.contentY = gridScroll.position
                    * Math.max(0, galleryGrid.contentHeight - galleryGrid.height)
        }
        interactive: true
        contentItem: Rectangle {
            implicitWidth: 6
            implicitHeight: 64
            radius: 3
            color: gridScroll.hovered || gridScroll.pressed
                ? Theme.accent : Theme.textFaint
            opacity: gridScroll.pressed ? 1.0 : gridScroll.hovered ? 0.85 : 0.5
            Behavior on color {
                ColorAnimation { duration: Theme.durFast }
            }
            Behavior on opacity {
                NumberAnimation { duration: Theme.durFast }
            }
        }
        background: Rectangle {
            implicitWidth: 6
            color: "transparent"
            radius: 3
        }
        padding: 0
    }
}
