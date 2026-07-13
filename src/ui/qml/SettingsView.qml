import QtQuick
import QtQuick.Layouts
import GameHQ
import "components"

Item {
    id: root
    signal closeRequested()

    property var categories: [
        { label: "General" }, { label: "Capture" }, { label: "Replay" },
        { label: "Input" }, { label: "Library" },
        { label: "Notifications & Sound" }, { label: "Advanced" }
    ]
    property int currentCategory: Math.max(0, Math.min(categories.length - 1,
        Number(app.config("ui.settings_category", 0))))

    function selectCategory(index, focusButton) {
        const next = Math.max(0, Math.min(categories.length - 1, index))
        currentCategory = next
        app.setConfig("ui.settings_category", next)
        if (focusButton && categoryRepeater.itemAt(next))
            categoryRepeater.itemAt(next).forceActiveFocus()
    }
    function activate() {
        if (categoryRepeater.itemAt(currentCategory))
            categoryRepeater.itemAt(currentCategory).forceActiveFocus()
    }
    function padCategoryStep(direction) {
        selectCategory((currentCategory + direction + categories.length) % categories.length, true)
        sounds.play("nav_tick")
    }
    function padFocusStep(direction) {
        const active = Window.window ? Window.window.activeFocusItem : null
        const next = active ? active.nextItemInFocusChain(direction > 0) : categoryRepeater.itemAt(currentCategory)
        if (next && next !== active) {
            next.forceActiveFocus()
            sounds.play("nav_tick")
        }
    }
    function padConfirm() {
        const active = Window.window ? Window.window.activeFocusItem : null
        if (active && active.clicked)
            active.clicked()
        else if (active && active.popup)
            active.popup.open()
    }

    onVisibleChanged: if (visible) Qt.callLater(activate)

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.s16

        Text {
            text: "Settings"
            color: Theme.text
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontDisplay
            font.weight: Font.Light
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Theme.s16

            Rectangle {
            Layout.preferredWidth: Theme.s48 * 4
            Layout.fillHeight: true
            radius: Theme.radiusL
            color: Theme.bg1
            border.width: 1
            border.color: Theme.stroke

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Theme.s8
                spacing: Theme.s4
                Repeater {
                    id: categoryRepeater
                    model: root.categories
                    delegate: SettingsCategoryButton {
                        Layout.fillWidth: true
                        label: modelData.label
                        selected: index === root.currentCategory
                        onClicked: root.selectCategory(index, true)
                        Keys.onUpPressed: root.selectCategory((index - 1 + root.categories.length) % root.categories.length, true)
                        Keys.onDownPressed: root.selectCategory((index + 1) % root.categories.length, true)
                        Keys.onRightPressed: root.padFocusStep(1)
                        Keys.onEscapePressed: root.closeRequested()
                    }
                }
                Item { Layout.fillHeight: true }
            }
        }

            StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: root.currentCategory
            GeneralSettingsPage {}
            CaptureSettingsPage {}
            ReplaySettingsPage {}
            InputSettingsPage {}
            LibrarySettingsPage {}
            FeedbackSettingsPage {}
            AdvancedSettingsPage {
                onRestoreAllRequested: resetAllDialog.open()
                onRestoreInputRequested: resetInputDialog.open()
                onRestoreCategoryRequested: function(category) {
                    restoreCategoryDialog.category = category
                    restoreCategoryDialog.open()
                }
            }
            }
        }
    }

    ConfirmDialog {
        id: resetAllDialog
        anchors.fill: parent
        z: 100
        title: "Restore all settings?"
        message: "Window preferences, capture behavior, replay options, notifications, and sound settings return to defaults. Captures and library data are not deleted."
        confirmLabel: "Restore defaults"
        onConfirmed: {
            app.resetAllConfig()
            root.currentCategory = 0
            sounds.play("confirm")
        }
    }

    ConfirmDialog {
        id: resetInputDialog
        anchors.fill: parent
        z: 100
        title: "Restore all input bindings?"
        message: "All controller, keyboard, and mouse overrides return to their built-in defaults."
        confirmLabel: "Restore defaults"
        onConfirmed: { input.bindingEditor.resetAllBindings(); sounds.play("confirm") }
    }

    ConfirmDialog {
        id: restoreCategoryDialog
        property string category: ""
        anchors.fill: parent
        z: 100
        title: "Restore " + category + " settings?"
        message: category + " options return to defaults. Captures and library data are not deleted."
        confirmLabel: "Restore defaults"
        onConfirmed: {
            app.resetCategory(restoreCategoryDialog.category)
            sounds.play("confirm")
        }
    }
}
