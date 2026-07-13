import QtQuick
import QtQuick.Controls.Basic as QC
import GameHQ

// Reusable Settings dropdown bound to a config.json key.
//
// `options` is an array of { label, value }. The stored config value is
// matched on load and every selection is persisted through app.setConfig —
// including selections made through the custom popup's delegate, which does
// NOT route through the ComboBox activate() path (the dev.52 pitfall that
// originally broke the replay-length dropdown).
QC.ComboBox {
    id: combo

    property var options: []
    property string configKey: ""
    property var defaultValue: undefined

    model: options
    textRole: "label"
    implicitWidth: Theme.s48 * 3 + Theme.s16
    font.family: Theme.fontFamily
    font.pixelSize: Theme.fontBody

    function commit(index) {
        currentIndex = index
        if (configKey.length > 0)
            app.setConfig(configKey, options[index].value)
    }

    function refresh() {
        var cur = configKey.length > 0 ? app.config(configKey, defaultValue) : defaultValue
        for (var i = 0; i < options.length; ++i)
            if (options[i].value === cur) { currentIndex = i; break }
    }
    Component.onCompleted: refresh()
    onActivated: commit(currentIndex)

    Connections {
        target: app
        function onConfigChanged(key, value) {
            if (key === combo.configKey) combo.refresh()
        }
        function onConfigGroupReset(prefix) {
            if (prefix.length === 0 || combo.configKey.startsWith(prefix + ".")) combo.refresh()
        }
    }

    contentItem: Text {
        text: combo.displayText
        color: Theme.text
        font: combo.font
        verticalAlignment: Text.AlignVCenter
        leftPadding: Theme.s12
        rightPadding: Theme.s24
        elide: Text.ElideRight
    }
    background: Rectangle {
        implicitHeight: Theme.fontBody + Theme.s16
        radius: Theme.radiusM
        color: Theme.surfaceAlt
        border.width: 1
        border.color: (combo.pressed || combo.popup.visible)
                      ? Theme.accent : Theme.stroke
        Behavior on border.color { ColorAnimation { duration: Theme.durFast } }
    }
    indicator: Text {
        x: combo.width - width - Theme.s12
        y: (combo.height - height) / 2
        text: "▾"
        color: Theme.textMuted
        font.pixelSize: Theme.fontBody
    }
    popup: QC.Popup {
        y: combo.height + Theme.s4
        width: combo.width
        implicitHeight: Math.min(contentItem.implicitHeight, Theme.s48 * 6)
        padding: 1
        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: combo.popup.visible ? combo.delegateModel : null
            currentIndex: combo.highlightedIndex
        }
        background: Rectangle {
            radius: Theme.radiusM
            color: Theme.surface
            border.width: 1
            border.color: Theme.stroke
        }
    }
    delegate: QC.ItemDelegate {
        width: combo.width
        highlighted: combo.highlightedIndex === index
        contentItem: Text {
            text: modelData.label
            color: Theme.text
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontBody
            verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle {
            color: highlighted ? Theme.surfaceAlt : "transparent"
        }
        // The custom popup's ListView doesn't route clicks through the
        // ComboBox's activate() path, so persist the choice here.
        onClicked: {
            combo.commit(index)
            combo.popup.close()
        }
    }
}
