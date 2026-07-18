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
    // ───────────────── Pad navigation ─────────────────
    // Settings is three panels: the app sidebar (owned by Main.qml), the
    // category list, and the options for the selected category. Left/Right
    // moves between panels, Up/Down moves inside the active one — so Up/Down
    // must never walk out of the panel it is in, which is exactly what a raw
    // nextItemInFocusChain() walk did.
    readonly property int panelCategories: 0
    readonly property int panelOptions: 1
    property int activePanel: panelCategories

    // The combo whose dropdown the pad opened. While this is set, Up/Down
    // drives the list instead of moving focus, and Cross/Circle commit/cancel.
    property var padCombo: null
    readonly property bool padComboOpen: padCombo !== null && padCombo.popup.visible

    function currentPage() {
        return pageStack.children[root.currentCategory] || null
    }
    function isInside(item, ancestor) {
        for (let p = item; p; p = p.parent)
            if (p === ancestor)
                return true
        return false
    }
    // First focusable control on the page — where Right lands when entering.
    function firstControlIn(page) {
        if (!page)
            return null
        let probe = page.nextItemInFocusChain(true)
        for (let guard = 0; probe && guard < 300; ++guard) {
            if (isInside(probe, page) && probe.activeFocusOnTab)
                return probe
            probe = probe.nextItemInFocusChain(true)
        }
        return null
    }
    function focusOptions() {
        const first = firstControlIn(currentPage())
        if (first) {
            first.forceActiveFocus()
            return true
        }
        return false   // a page with nothing to focus keeps the caller on categories
    }

    function enterPanel(panel) {
        if (panel === panelOptions) {
            if (!focusOptions())
                return false
            activePanel = panelOptions
        } else {
            activePanel = panelCategories
            activate()
        }
        sounds.play("nav_tick")
        return true
    }

    function padCategoryStep(direction) {
        selectCategory((currentCategory + direction + categories.length) % categories.length, true)
        sounds.play("nav_tick")
    }

    // Up/Down inside the options panel: walk the focus chain but refuse to
    // leave the current page, so focus cannot escape into the category list
    // or the sidebar.
    function optionsStep(direction) {
        const page = currentPage()
        const active = Window.window ? Window.window.activeFocusItem : null
        if (!page || !active || !isInside(active, page)) {
            focusOptions()
            return
        }
        let next = active.nextItemInFocusChain(direction > 0)
        for (let guard = 0; next && next !== active && guard < 300; ++guard) {
            if (isInside(next, page) && next.activeFocusOnTab)
                break
            next = next.nextItemInFocusChain(direction > 0)
        }
        if (next && next !== active && isInside(next, page)) {
            next.forceActiveFocus()
            sounds.play("nav_tick")
        }
    }

    function padFocusStep(direction) {
        if (padComboOpen) {
            padCombo.padStep(direction)
            sounds.play("nav_tick")
            return
        }
        if (activePanel === panelOptions)
            optionsStep(direction)
        else
            padCategoryStep(direction)
    }

    // Left/Right between panels. Returns false when there is nothing further
    // left, so Main.qml can hand focus back to the app sidebar.
    function padPanelStep(direction) {
        if (padComboOpen)
            return true   // the dropdown owns Left/Right too; swallow it
        if (direction > 0)
            return activePanel === panelCategories ? enterPanel(panelOptions) : true
        if (activePanel === panelOptions) {
            enterPanel(panelCategories)
            return true
        }
        return false   // already leftmost — caller moves to the sidebar
    }

    function padConfirm() {
        if (padComboOpen) {
            padCombo.padCommitHighlighted()
            padCombo.popup.close()
            padCombo = null
            sounds.play("confirm")
            return
        }
        const active = Window.window ? Window.window.activeFocusItem : null
        if (!active)
            return
        // A combo opens its list; anything else (toggle, button) just fires.
        if (active.popup && active.padStep) {
            padCombo = active
            active.padBeginHighlight()
            active.popup.open()
            sounds.play("nav_tick")
        } else if (active.clicked) {
            active.clicked()
        }
    }

    // Circle: close the dropdown without committing, else step back a panel.
    // Returns false when there is nothing left to back out of, so Main.qml
    // closes Settings.
    function padBack() {
        if (padComboOpen) {
            padCombo.popup.close()
            padCombo = null
            return true
        }
        if (activePanel === panelOptions) {
            enterPanel(panelCategories)
            return true
        }
        return false
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
            id: pageStack
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
