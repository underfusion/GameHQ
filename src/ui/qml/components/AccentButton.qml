import QtQuick
import QtQuick.Layouts
import GameHQ

// Primary (gradient) / secondary (surface) button per design-system §6.
Rectangle {
    id: root
    property string label
    property string icon: ""
    property string iconFontFamily: Theme.fontFamily
    property color iconColor: primary ? "#FFFFFF" : Theme.text
    property color labelColor: primary ? "#FFFFFF" : Theme.text
    // Accent-tinted, not a neutral gray hairline: secondary buttons need to read
    // as buttons on their own, without depending on what surface they sit on.
    property color borderColor: Theme.accent
    property bool primary: false
    // Low-emphasis toolbar action: soft surface gradient at rest, with its
    // outline appearing only on hover instead of reading as pre-selected.
    property bool quiet: false
    property color quietTopColor: Theme.surface
    property color quietBottomColor: Theme.bg1
    property color quietIdleBorderColor: Theme.borderLight
    signal clicked()

    implicitWidth: content.width + Theme.s24 * 2
    implicitHeight: 36
    // Explicit, not just implicit*: QtQuick Layouts children (RowLayout/ColumnLayout)
    // otherwise collapse this to zero size instead of falling back to implicitWidth/Height.
    Layout.preferredWidth: implicitWidth
    Layout.preferredHeight: implicitHeight
    radius: Theme.radiusS
    color: primary ? Theme.accent1
                   : (quiet ? "transparent" : (mouse.containsMouse ? Theme.surfaceAlt : Theme.surface))
    border.width: primary ? 0 : (quiet ? 1 : 1.5)
    border.color: quiet && !mouse.containsMouse ? Qt.lighter(quietIdleBorderColor, 1.25)
                                               : borderColor

    scale: root.enabled && mouse.containsMouse ? 1.04 : 1.0
    Behavior on scale { NumberAnimation { duration: Theme.durFast; easing.type: Easing.OutQuad } }

    // Separate child rather than root's own gradient property, so the fill is never
    // dependent on how root's color/gradient/border interact.
    Rectangle {
        visible: root.primary
        anchors.fill: parent
        radius: parent.radius
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0; color: Theme.accent1 }
            GradientStop { position: 1; color: Theme.accent2 }
        }
    }

    Rectangle {
        visible: root.quiet && !root.primary
        anchors.fill: parent
        anchors.margins: root.border.width
        radius: Math.max(0, parent.radius - root.border.width)
        gradient: Gradient {
            orientation: Gradient.Vertical
            GradientStop {
                position: 0
                color: Qt.lighter(root.quietTopColor, mouse.containsMouse ? 1.28 : 1.18)
            }
            GradientStop {
                position: 1
                color: Qt.lighter(root.quietBottomColor, mouse.containsMouse ? 1.22 : 1.13)
            }
        }
    }

    Rectangle { // hover highlight, same treatment for primary and secondary
        anchors.fill: parent
        radius: parent.radius
        color: "#FFFFFF"
        opacity: root.enabled && mouse.containsMouse && !mouse.pressed ? 0.1 : 0
        Behavior on opacity { NumberAnimation { duration: Theme.durFast } }
    }

    Rectangle { // brief flash on click, on top of the press-opacity dip below
        id: clickFlash
        anchors.fill: parent
        radius: parent.radius
        color: "#FFFFFF"
        opacity: 0
    }

    opacity: !root.enabled ? 0.4 : (mouse.pressed ? 0.85 : 1.0)
    Behavior on opacity { NumberAnimation { duration: Theme.durFast } }

    activeFocusOnTab: true
    Keys.onReturnPressed: root.clicked()
    Keys.onSpacePressed: root.clicked()

    Row {
        id: content
        anchors.centerIn: parent
        spacing: root.icon === "" ? 0 : Theme.s8

        Text {
            visible: root.icon !== ""
            anchors.verticalCenter: parent.verticalCenter
            text: root.icon
            color: root.iconColor
            font.family: root.iconFontFamily
            font.pixelSize: Theme.fontBody
        }

        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: root.label
            color: root.labelColor
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontBody
        }
    }

    Rectangle { // focus ring
        anchors.fill: parent
        anchors.margins: -3
        radius: parent.radius + 3
        color: "transparent"
        border.width: root.activeFocus && !root.quiet ? 2 : 0
        border.color: Theme.accent
    }

    SequentialAnimation {
        id: clickFlashAnim
        NumberAnimation { target: clickFlash; property: "opacity"; to: 0.3; duration: 70 }
        NumberAnimation { target: clickFlash; property: "opacity"; to: 0; duration: 200 }
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        onClicked: {
            clickFlashAnim.restart()
            root.clicked()
        }
    }
}
