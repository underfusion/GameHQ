import QtQuick
import QtQuick.Layouts
import GameHQ

Item {
    id: root
    default property alias contentData: page.data

    Flickable {
        id: flick
        anchors.fill: parent
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        contentWidth: width
        contentHeight: page.implicitHeight + Theme.s16

        // Row labels, section headers and empty space are plain Text/Rectangles
        // — none of them focusable. Without this, a control the user clicked
        // (e.g. the theme combo) keeps keyboard focus after they click away, and
        // goes on consuming Up/Down for the rest of the session: the arrows
        // would still be reskinning the app from anywhere on the page. Clicking
        // any non-interactive part of the page hands focus back to the page, so
        // the arrows only drive a control while that control is focused.
        // Sits behind the content (z: -1) so real controls are hit first; the
        // Flickable still filters drags, so flicking keeps working.
        MouseArea {
            width: flick.contentWidth
            height: Math.max(flick.height, flick.contentHeight)
            z: -1
            onClicked: root.forceActiveFocus()
        }

        ColumnLayout {
            id: page
            width: Math.max(0, flick.width - Theme.s16)
            spacing: Theme.s24
        }
    }
}
