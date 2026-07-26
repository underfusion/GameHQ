import QtQuick
import QtQuick.Controls.Basic as QC
import GameHQ

QC.AbstractButton {
    id: root
    property string configKey: ""
    property bool defaultValue: false
    width: Theme.s48 - Theme.s4
    height: Theme.s24
    checkable: true
    focusPolicy: Qt.StrongFocus

    function refresh() {
        checked = configKey.length > 0 ? app.config(configKey, defaultValue) : defaultValue
    }
    Component.onCompleted: refresh()
    onClicked: if (configKey.length > 0) app.setConfig(configKey, checked)

    Connections {
        target: app
        function onConfigChanged(key, value) {
            if (key === root.configKey) root.checked = Boolean(value)
        }
        function onConfigGroupReset(prefix) {
            if (prefix.length === 0 || root.configKey.startsWith(prefix + ".")) root.refresh()
        }
    }

    background: Rectangle {
        radius: Theme.radiusPill
        color: root.checked ? Theme.accent : Theme.surfaceAlt
        border.width: 1
        border.color: root.activeFocus ? Theme.accent2 : Theme.stroke
        Behavior on color { ColorAnimation { duration: Theme.durFast } }
        Rectangle {
            width: Theme.s24 - Theme.s4 - 2
            height: width
            radius: Theme.radiusPill
            anchors.verticalCenter: parent.verticalCenter
            x: root.checked ? parent.width - width - Theme.s4 + 1 : Theme.s4 - 1
            color: Theme.text
            Behavior on x { NumberAnimation { duration: Theme.durFast; easing.type: Easing.OutCubic } }
        }
    }
}
