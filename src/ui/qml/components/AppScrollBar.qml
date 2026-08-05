import QtQuick
import QtQuick.Controls.Basic as QC
import GameHQ

// Shared themed scrollbar for every scrollable viewport (settings pages,
// dialogs, help, release notes). Visible whenever the content actually
// overflows — not only while scrolling — so a pad user can tell a section
// scrolls before moving. Visuals match the gallery grid's scrollbar.
QC.ScrollBar {
    id: bar
    policy: bar.size > 0 && bar.size < 1 ? QC.ScrollBar.AlwaysOn
                                         : QC.ScrollBar.AlwaysOff
    minimumSize: 0.08
    padding: 0
    contentItem: Rectangle {
        implicitWidth: 6
        implicitHeight: 64
        radius: 3
        color: bar.hovered || bar.pressed ? Theme.accent : Theme.textFaint
        opacity: bar.pressed ? 1.0 : bar.hovered ? 0.85 : 0.5
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
}
