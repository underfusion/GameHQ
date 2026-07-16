import QtQuick
import QtQuick.Layouts
import GameHQ

// Read-only box displaying a configured capture root. One per configurable
// location on the Capture settings page.
//
// The outline is `borderLight`, not the `stroke` used by SettingsSection's
// panel: this is an inner field sitting on that panel and needs the stronger
// hairline to read as its own element.
Rectangle {
    id: root
    // The path to show. Elides in the middle — the tail of a path is the
    // informative part, so the start goes first.
    property alias text: pathText.text

    Layout.fillWidth: true
    implicitHeight: pathText.implicitHeight + Theme.s8 * 2
    radius: Theme.radiusM
    color: Theme.bg1
    border.width: 1
    border.color: Theme.borderLight

    Text {
        id: pathText
        anchors.fill: parent
        anchors.margins: Theme.s8
        color: Theme.textMuted
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontCaption
        horizontalAlignment: Text.AlignRight
        elide: Text.ElideMiddle
    }
}
