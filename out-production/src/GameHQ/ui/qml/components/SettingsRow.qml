import QtQuick
import QtQuick.Layouts
import GameHQ

Item {
    id: root

    property string icon: ""
    property string label: ""
    property string description: ""
    property string tone: "normal" // normal | warning | danger | success
    property bool showDivider: true
    property bool compact: false
    property bool stackOnNarrowWidth: true
    property bool highlighted: false
    property int controlWidth: Theme.s48 * 4
    property int stackBreakpoint: 660
    default property alias controlData: controlHost.data

    readonly property int responsiveBreakpoint: Math.max(stackBreakpoint,
                                                          controlHost.implicitWidth + 360)
    readonly property bool narrow: stackOnNarrowWidth && width > 0
                                   && width < responsiveBreakpoint
    readonly property int verticalPadding: compact ? Theme.s8 : Theme.s12
    readonly property color toneColor: tone === "warning" ? Theme.warning
                                       : tone === "danger" ? Theme.danger
                                       : tone === "success" ? Theme.success
                                                            : Theme.text

    Layout.fillWidth: true
    Layout.minimumWidth: 0
    implicitHeight: (narrow
                     ? textHost.implicitHeight + Theme.s8 + controlHost.implicitHeight
                     : Math.max(textHost.implicitHeight, controlHost.implicitHeight))
                    + verticalPadding * 2

    Rectangle {
        anchors.fill: parent
        anchors.leftMargin: -Theme.s8
        anchors.rightMargin: -Theme.s8
        radius: Theme.radiusS
        color: root.highlighted ? Theme.surfaceHover : "transparent"
    }

    RowLayout {
        id: textHost
        anchors.left: parent.left
        anchors.right: root.narrow ? parent.right : controlHost.left
        anchors.rightMargin: root.narrow ? 0 : Theme.s16
        y: root.verticalPadding
        height: implicitHeight
        spacing: Theme.s12

        Rectangle {
            visible: root.icon.length > 0
            Layout.preferredWidth: 30
            Layout.preferredHeight: 30
            radius: Theme.radiusS
            color: root.tone === "warning" ? Theme.warningSoft
                 : root.tone === "danger" ? Theme.dangerSoft
                 : root.tone === "success" ? Theme.successSoft
                                           : Theme.accentSoft
            Text {
                anchors.centerIn: parent
                text: root.icon
                color: root.tone === "normal" ? Theme.accent : root.toneColor
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontBody
                font.weight: Font.DemiBold
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            spacing: Theme.s4
            Text {
                text: root.label
                color: root.toneColor
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontBody
                font.weight: Font.Medium
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                Layout.minimumWidth: 0
            }
            Text {
                visible: root.description.length > 0
                text: root.description
                color: Theme.textMuted
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontCaption
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                Layout.minimumWidth: 0
            }
        }
    }

    RowLayout {
        id: controlHost
        anchors.right: parent.right
        y: root.narrow
           ? textHost.y + textHost.height + Theme.s8
           : Math.round((root.height - height) / 2)
        width: implicitWidth
        height: implicitHeight
        spacing: Theme.s8
    }

    Rectangle {
        visible: root.showDivider
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 1
        color: Theme.divider
    }
}
