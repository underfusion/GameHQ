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
        ColumnLayout {
            id: page
            width: Math.max(0, flick.width - Theme.s16)
            spacing: Theme.s24
        }
    }
}
