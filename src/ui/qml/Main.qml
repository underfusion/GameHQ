import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Window
import GameHQ
import "components"
import "helpers/SidebarCategories.js" as SidebarCategories

// Desktop gallery window: dark gradient background,
// thin display type, sidebar → grid → preview.
ApplicationWindow {
    id: window
    visible: !app.startMinimized
    width: app ? app.config("ui.window_width", 1280) : 1280
    height: app ? app.config("ui.window_height", 760) : 760
    x: {
        var sx = app ? app.config("ui.window_x", -1) : -1
        return sx >= 0 ? sx : Math.max(0, Math.round((Screen.width - width) / 2))
    }
    y: {
        var sy = app ? app.config("ui.window_y", -1) : -1
        return sy >= 0 ? sy : Math.max(0, Math.round((Screen.height - height) / 2))
    }
    minimumWidth: 1024
    minimumHeight: 640
    title: Brand.name
    // ThemeBackdrop paints the window; this stays as the fill behind it so a
    // resize never flashes the default white.
    color: Theme.bg0

    ThemeBackdrop {
        anchors.fill: parent
        z: -1
    }

    // Debounce timer for geometry persistence (avoid writing config.json on
    // every pixel of a drag). Fires 500ms after the last resize/move.
    Timer {
        id: persistTimer
        interval: 500
        repeat: false
        onTriggered: window.doPersistGeometry()
    }

    BulkSelection {
        id: bulkSelection
        galleryModel: app.gallery
    }

    onWidthChanged: window.persistGeometry()
    onHeightChanged: window.persistGeometry()
    onXChanged: window.persistGeometry()
    onYChanged: window.persistGeometry()

    property bool settingsOpen: false
    property bool helpOpen: false

    // DualSense support (0.3/0.6 extended to the desktop window): L1 is the
    // ONLY entry to the sidebar (left panel), R1 is the ONLY way back to the
    // grid (right panel) — D-pad/arrow LEFT at the leftmost thumbnail no
    // longer enters the sidebar, it wraps to the previous row instead. Inside
    // either region D-pad/stick (or arrow keys) drives the cursor; in the grid
    // it row-wraps (right edge → next row left; left edge → prev row right).
    // Cross opens the lightbox (grid) or selects the row (sidebar), Circle
    // backs out, Triangle favorites, Square opens the action menu.
    // Pad input only reaches here while this window has real OS focus (see
    // onActiveChanged below) — never steals the pad from a focused game.
    property var sidebarCategories: SidebarCategories.categories(app.currentGameAvailable)
    property bool menuOpen: false
    property int menuIndex: 0
    property bool bulkMode: false
    // Tracks which input source was used last so the footer hint bar can
    // render matching labels (keyboard glyphs vs DualSense button names).
    // Flipped to true on any pad input, false on any key press.
    property bool usingGamepad: false
    // Sidebar focus region: when true the "cursor" has left the grid (L1 from
    // the pad — the only entry path) and now lives in the sidebar. UP/DOWN
    // walks the flat sidebar list (categories → games → Settings → Help),
    // R1 (or activating an entry, or Esc) returns focus to the grid.
    property bool sidebarFocused: false
    // Flat index into the sidebar list while sidebarFocused. Layout:
    //   0 .. catCount-1                                  → categories
    //   catCount .. catCount+gameCount-1                 → games
    //   catCount+gameCount                               → Settings
    //   catCount+gameCount+1                             → Help
    property int sidebarHoverIndex: 0

    // Gallery zoom target — the *ideal* tile size in px (160 → 480). The actual
    // cellWidth is grid.width / columns so tiles always fill the full width with
    // no right-side gap. +/- step to the exact next column boundary (no dead
    // clicks), the slider maps continuously. Persisted to config.json under
    // "ui.zoom_level" and restored on startup.
    property int zoomLevel: 280

    // Min/max columns for a given width, derived from the 160/480 tile limits.
    function minColumns(w) {
        if (w <= 0) return 1
        return Math.max(1, Math.floor(w / 480))
    }
    function maxColumns(w) {
        if (w <= 0) return 1
        return Math.max(1, Math.floor(w / 160))
    }

    // Number of columns the grid shows for the given width + current zoom target.
    function gridColumns(w) {
        if (w <= 0)
            return 1
        return Math.max(1, Math.round(w / window.zoomLevel))
    }

    // Zoom IN = bigger tiles = one fewer column. Steps to the exact zoomLevel
    // that yields (curCols − 1) so every click is guaranteed to change the grid.
    function zoomIn() {
        var w = grid.width
        if (w <= 0) return
        var cur = gridColumns(w)
        if (cur <= minColumns(w)) return
        window.zoomLevel = Math.round(w / (cur - 1))
        sounds.play("nav_tick")
    }
    // Zoom OUT = smaller tiles = one more column. Same boundary-stepping logic.
    function zoomOut() {
        var w = grid.width
        if (w <= 0) return
        var cur = gridColumns(w)
        if (cur >= maxColumns(w)) return
        window.zoomLevel = Math.round(w / (cur + 1))
        sounds.play("nav_tick")
    }

    // Debounced persist of window geometry + zoom to config.json so the app
    // remembers its size/position across restarts.
    function persistGeometry() {
        persistTimer.restart()
    }
    function doPersistGeometry() {
        app.setConfig("ui.window_width", window.width)
        app.setConfig("ui.window_height", window.height)
        app.setConfig("ui.window_x", window.x)
        app.setConfig("ui.window_y", window.y)
        app.setConfig("ui.zoom_level", window.zoomLevel)
    }

    onActiveChanged: input.setDesktopFocused(window.active || lightbox.active)
    // OS-level minimize (title bar button or taskbar): when enabled, drop
    // straight to the tray instead of leaving a taskbar entry. showWindow()
    // (tray click / gallery open) restores to the normal windowed state.
    onVisibilityChanged: function(visibility) {
        if (visibility === Window.Minimized && app.config("tray.minimize_to_tray", false))
            window.hide()
    }
    onClosing: function(close) {
        window.doPersistGeometry()
        if (app.config("tray.close_to_tray", true)) {
            close.accepted = false
            window.hide()
        } else {
            close.accepted = true
            app.quitApplication()
        }
    }
    Component.onCompleted: {
        input.setDesktopFocused(window.active)
        // Restore saved zoom level (if any) from config.json.
        var savedZoom = app.config("ui.zoom_level", 0)
        if (savedZoom >= 160 && savedZoom <= 480)
            window.zoomLevel = savedZoom
    }
    onZoomLevelChanged: window.persistGeometry()

    function currentTabIndex() {
        if (app.gameId >= 0) {
            if (app.currentGameAvailable && app.gameId === app.currentGameId) {
                const currentGameKey = app.category === "favorites" ? "game_favorites" : "game"
                const currentGameIdx = window.sidebarCategories.findIndex(c => c.key === currentGameKey)
                return currentGameIdx >= 0 ? currentGameIdx : 0
            }
            const idx = app.games.findIndex(g => g.id === app.gameId)
            return idx >= 0 ? window.sidebarCategories.length + idx : 0
        }
        const idx = window.sidebarCategories.findIndex(c => c.key === app.category)
        return idx >= 0 ? idx : 0
    }

    // The selection algebra lives in BulkSelection.qml; these stay as the window
    // API every caller (grid, header, pad handlers) already binds to.
    function bulkCount() {
        return bulkSelection.count()
    }

    function bulkIsChecked(path) {
        return bulkSelection.isChecked(path)
    }

    function bulkAllSelected() {
        return bulkSelection.allSelected()
    }

    function bulkToggle(index, extendRange) {
        if (bulkSelection.toggle(index, extendRange))
            sounds.play("nav_tick")
    }

    function bulkClear() {
        bulkSelection.clear()
    }

    function bulkEnter() {
        if (grid.count === 0)
            return
        window.menuOpen = false
        window.sidebarFocused = false
        window.bulkMode = true
        window.bulkClear()
        grid.forceActiveFocus()
    }

    function bulkExit() {
        window.bulkMode = false
        window.bulkClear()
        grid.forceActiveFocus()
    }

    // Settings from a button rather than the sidebar row: same end state the
    // sidebar produces, minus the sidebar cursor.
    function openSettings() {
        if (window.settingsOpen)
            return
        if (window.bulkMode)
            window.bulkExit()
        window.menuOpen = false
        window.helpOpen = false
        window.settingsOpen = true
        window.sidebarFocused = false
        sounds.play("nav_tick")
    }

    function padBulkToggle() {
        if (window.bulkMode)
            window.bulkExit()
        else
            window.bulkEnter()
        sounds.play("nav_tick")
    }

    function bulkSelectAll() {
        sounds.play(bulkSelection.selectAll() ? "nav_tick" : "favorite")
    }

    function bulkRows() {
        return bulkSelection.rows()
    }

    function bulkAskDelete() {
        var n = window.bulkCount()
        if (n === 0)
            return
        bulkDeleteDialog.message = n + " capture" + (n === 1 ? "" : "s")
            + " will be permanently deleted.\nThis cannot be undone."
        bulkDeleteDialog.open()
    }

    function bulkConfirmDelete() {
        var rows = window.bulkRows()
        if (rows.length === 0) {
            window.bulkExit()
            return
        }
        sounds.play("confirm")
        app.deleteCaptures(rows)
        window.bulkExit()
    }

    // ───────────────── Flat sidebar list helpers (sidebarFocused mode) ─────────────────
    function sidebarFlatCount() {
        return window.sidebarCategories.length + app.games.length + 2  // +Settings +Help
    }

    function refreshSidebarHoverIndex() {
        // Snap the cursor to the row that's currently active so entering the
        // sidebar always lands on the row the user is already looking at.
        if (window.settingsOpen) {
            window.sidebarHoverIndex = window.sidebarCategories.length + app.games.length
        } else if (window.helpOpen) {
            window.sidebarHoverIndex = window.sidebarCategories.length + app.games.length + 1
        } else {
            window.sidebarHoverIndex = window.currentTabIndex()
        }
    }

    function activateSidebarRow(i) {
        const catCount = window.sidebarCategories.length
        const gameCount = app.games.length
        if (i < catCount) {
            window.settingsOpen = false; window.helpOpen = false
            const f = SidebarCategories.resolveFilter(window.sidebarCategories[i].key,
                                                      app.currentGameId)
            app.setGameCategory(f.category, f.gameId)
        } else if (i < catCount + gameCount) {
            window.settingsOpen = false; window.helpOpen = false
            app.setGame(app.games[i - catCount].id)
        } else if (i === catCount + gameCount) {
            window.helpOpen = false; window.settingsOpen = true
        } else {  // Help
            window.settingsOpen = false; window.helpOpen = true
        }
        sounds.play("nav_tick")
        // Always drop back to the grid — for Settings/Help this means the
        // sidebar cursor is gone while the panel is up, and the user is back
        // in the grid when they close it.
        window.sidebarFocused = false
        grid.forceActiveFocus()
    }

    function sidebarStepVertical(direction) {
        const total = window.sidebarFlatCount()
        if (total <= 0)
            return
        window.sidebarHoverIndex = (window.sidebarHoverIndex + direction + total) % total
        sounds.play("nav_tick")
    }

    // Column count of the visible grid — used to detect column edges so the
    // grid row-wraps (rightmost → next row leftmost, leftmost → prev row rightmost).
    function gridColumnCount() {
        return window.gridColumns(grid.width)
    }

    function padTabStep(direction) {
        if (window.settingsOpen) {
            settingsView.padCategoryStep(direction)
            return
        }
        if (window.menuOpen || window.bulkMode || window.helpOpen)
            return
        // L1/R1 no longer cycles tabs inside the sidebar — instead it jumps
        // focus between the two panels: L1 → left panel (sidebar, controller
        // driven via D-pad ↑↓), R1 → right panel (thumbnail grid, works as
        // before). Up/Down already does the right thing per region (see
        // padNavigateVertical), so this just flips which region is "active".
        if (direction < 0) {
            // L1: enter the sidebar (left panel). No-op if already there.
            if (window.sidebarFocused)
                return
            window.sidebarFocused = true
            window.refreshSidebarHoverIndex()
        } else {
            // R1: enter the thumbnail grid (right panel). No-op if already there.
            if (!window.sidebarFocused)
                return
            window.sidebarFocused = false
            grid.forceActiveFocus()
        }
        sounds.play("nav_tick")
    }

    function padNavigate(direction) {
        // Settings is three panels — sidebar │ categories │ options — and
        // Left/Right walks between them. Right off the sidebar enters the
        // categories; Left past the categories returns to the sidebar.
        if (window.settingsOpen) {
            if (window.sidebarFocused) {
                if (direction > 0) {
                    window.sidebarFocused = false
                    settingsView.enterPanel(settingsView.panelCategories)
                }
                return
            }
            if (!settingsView.padPanelStep(direction)) {
                window.sidebarFocused = true
                window.refreshSidebarHoverIndex()
                sounds.play("nav_tick")
            }
            return
        }
        if (window.menuOpen || window.helpOpen)
            return
        if (deleteDialog.visible || bulkDeleteDialog.visible)
            return
        if (window.sidebarFocused) {
            // L1/R1 are the only way in/out of the sidebar (see padTabStep);
            // D-pad LEFT/RIGHT inside the sidebar is a no-op.
            return
        }
        // Refresh the nav-lock so hover-follow can't override this move.
        grid._navLockUntil = Date.now() + 250
        const cols = window.gridColumnCount()
        const col = grid.currentIndex % cols
        if (direction < 0) {
            // LEFT at the leftmost column wraps to the previous row's
            // rightmost column; at index 0 it clamps (no wrap).
            if (grid.currentIndex === 0)
                return
            if (col === 0)
                grid.currentIndex = grid.currentIndex - 1
            else
                grid.moveCurrentIndexLeft()
        } else {
            // RIGHT past the rightmost column wraps to the next row's
            // leftmost column; at the last item it clamps (no wrap).
            if (grid.currentIndex === grid.count - 1)
                return
            if (col === cols - 1)
                grid.currentIndex = grid.currentIndex + 1
            else
                grid.moveCurrentIndexRight()
        }
    }

    function padNavigateVertical(direction) {
        if (window.settingsOpen) {
            // Up/Down moves inside whichever panel is focused, never across.
            if (window.sidebarFocused)
                window.sidebarStepVertical(direction)
            else
                settingsView.padFocusStep(direction)
            return
        }
        if (window.helpOpen)
            return
        if (deleteDialog.visible || bulkDeleteDialog.visible)
            return
        if (window.menuOpen) {
            const menuCount = padMenu.entries.length
            window.menuIndex = (window.menuIndex + direction + menuCount) % menuCount
            sounds.play("nav_tick")
        } else if (window.sidebarFocused) {
            window.sidebarStepVertical(direction)
        } else if (grid.count === 0) {
            return
        } else {
            // Refresh the nav-lock so hover-follow can't override this move.
            grid._navLockUntil = Date.now() + 250
            // Explicit boundary clamp so spurious events (or a stuck analog
            // stick reading) can't wrap or jump: no item above the first row,
            // no item below the last row.
            const cols = window.gridColumnCount()
            if (direction < 0) {
                if (grid.currentIndex < cols)
                    return
                grid.moveCurrentIndexUp()
            } else {
                // currentIndex + cols >= count means no item sits directly
                // below (handles partial last rows too).
                if (grid.currentIndex + cols >= grid.count)
                    return
                grid.moveCurrentIndexDown()
            }
        }
    }

    function padConfirm() {
        if (deleteDialog.visible) { deleteDialog.confirmed(); deleteDialog.close(); return }
        if (bulkDeleteDialog.visible) { window.bulkConfirmDelete(); bulkDeleteDialog.close(); return }
        if (window.settingsOpen) {
            // On the sidebar, Cross still activates the sidebar row (it is the
            // app's nav, not a settings control).
            if (window.sidebarFocused)
                window.activateSidebarRow(window.sidebarHoverIndex)
            else
                settingsView.padConfirm()
            return
        }
        if (window.helpOpen)
            return
        if (window.bulkMode) {
            const rec = app.gallery.get(grid.currentIndex)
            if (rec.filePath)
                window.bulkToggle(grid.currentIndex, false)
            return
        }
        if (window.menuOpen) {
            window.padMenuConfirm()
        } else if (window.sidebarFocused) {
            window.activateSidebarRow(window.sidebarHoverIndex)
        } else {
            sounds.play("confirm")
            lightbox.openAt(grid.currentIndex)
        }
    }

    function padFavorite() {
        if (window.menuOpen || window.settingsOpen || window.helpOpen)
            return
        if (window.sidebarFocused)
            return  // favorites only act on a grid tile
        if (window.bulkMode) {
            window.bulkSelectAll()
            return
        }
        sounds.play("favorite")
        app.toggleFavorite(grid.currentIndex)
    }

    function padToggleMenu() {
        if (window.settingsOpen || grid.count === 0)
            return
        if (window.sidebarFocused)
            return  // action menu only acts on a grid tile
        if (window.bulkMode) {
            window.bulkAskDelete()
            return
        }
        window.menuOpen = !window.menuOpen
        if (window.menuOpen)
            window.menuIndex = 0
    }

    function padMenuConfirm() {
        // Index order mirrors padMenu.entries.
        if (window.menuIndex === 0) {
            app.showInFolder(grid.currentIndex)
        } else if (window.menuIndex === 1) {
            const rec = app.gallery.get(grid.currentIndex)
            window.askDelete(grid.currentIndex, rec.gameName, rec.dateText)
        } else {
            window.menuOpen = false
            window.bulkEnter()
            sounds.play("confirm")
            return
        }
        sounds.play("confirm")
        window.menuOpen = false
    }

    function padBack() {
        if (deleteDialog.visible) { deleteDialog.canceled(); deleteDialog.close(); return }
        if (bulkDeleteDialog.visible) { bulkDeleteDialog.canceled(); bulkDeleteDialog.close(); return }
        if (window.settingsOpen) {
            // Circle unwinds one step at a time: dropdown → options →
            // categories → sidebar → out of Settings entirely.
            if (!window.sidebarFocused && settingsView.padBack())
                return
            if (!window.sidebarFocused) {
                window.sidebarFocused = true
                window.refreshSidebarHoverIndex()
                return
            }
            window.settingsOpen = false
            window.sidebarFocused = false
            grid.forceActiveFocus()
        } else if (window.bulkMode) {
            window.bulkExit()
        } else if (window.sidebarFocused) {
            window.sidebarFocused = false
            grid.forceActiveFocus()
        } else if (window.menuOpen) {
            window.menuOpen = false
        } else if (lightbox.visible) {
            lightbox.close()
        }
    }

    Connections {
        target: input
        function onDesktopNavigate(direction) {
            window.usingGamepad = true
            if (lightbox.visible) {
                lightbox.padNavigate(direction)
                return
            }
            window.padNavigate(direction)
        }
        function onDesktopNavigateVertical(direction) {
            window.usingGamepad = true
            if (lightbox.visible) {
                lightbox.padReveal()
                return
            }
            window.padNavigateVertical(direction)
        }
        function onDesktopConfirm() {
            window.usingGamepad = true
            if (lightbox.visible) {
                lightbox.padConfirm()
                return
            }
            window.padConfirm()
        }
        function onDesktopFavorite() {
            window.usingGamepad = true
            if (lightbox.visible) {
                lightbox.padReveal()
                return
            }
            window.padFavorite()
        }
        function onDesktopMenu() {
            window.usingGamepad = true
            if (lightbox.visible) {
                lightbox.padReveal()
                return
            }
            window.padToggleMenu()
        }
        function onDesktopTabStep(direction) {
            window.usingGamepad = true
            if (lightbox.visible) {
                lightbox.padStep(direction)
                return
            }
            window.padTabStep(direction)
        }
        function onDesktopBack() { window.usingGamepad = true; window.padBack() }
        function onDesktopSettings() {
            window.usingGamepad = true
            if (lightbox.visible)
                return
            window.openSettings()
        }
        function onDesktopZoom(direction) {
            window.usingGamepad = true
            if (lightbox.visible || window.settingsOpen || window.helpOpen)
                return
            if (direction > 0)
                window.zoomIn()
            else
                window.zoomOut()
        }
        function onDesktopBulkToggle() {
            window.usingGamepad = true
            if (lightbox.visible || window.settingsOpen || window.helpOpen)
                return
            window.padBulkToggle()
        }
        function onPlaybackPlayPause() {
            if (lightbox.visible)
                lightbox.toggleVideoPlayback()
        }
        function onPlaybackSeek(direction) {
            if (lightbox.visible)
                lightbox.padNavigate(direction)
        }
    }

    Connections {
        target: lightbox
        function onActiveChanged() {
            input.setDesktopFocused(window.active || lightbox.active)
        }
        function onVisibleChanged() {
            input.setPlaybackActive(lightbox.visible
                                    && lightbox.current.captureType === "video")
        }
        function onIndexChanged() {
            input.setPlaybackActive(lightbox.visible
                                    && lightbox.current.captureType === "video")
        }
    }

    // Background gradient (design-system §1: never flat black)
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0; color: Theme.bg1 }
            GradientStop { position: 1; color: Theme.bg0 }
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: Theme.s16
        spacing: Theme.s16

        // ───────────────────────── Sidebar ─────────────────────────
        DesktopSidebar {
            categories: window.sidebarCategories
            settingsOpen: window.settingsOpen
            helpOpen: window.helpOpen
            sidebarFocused: window.sidebarFocused
            sidebarHoverIndex: window.sidebarHoverIndex
            onPageClosed: {
                window.settingsOpen = false
                window.helpOpen = false
            }
            onSettingsRequested: {
                window.helpOpen = false
                window.settingsOpen = true
            }
            onHelpRequested: {
                window.settingsOpen = false
                window.helpOpen = true
            }
        }
        // ───────────────────────── Settings ─────────────────────────
        SettingsView {
            id: settingsView
            visible: window.settingsOpen
            Layout.fillWidth: true
            Layout.fillHeight: true
            onCloseRequested: {
                window.settingsOpen = false
                grid.forceActiveFocus()
            }
        }

        // ───────────────────────── Help ─────────────────────────
        HelpView {
            visible: window.helpOpen
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        // ───────────────────────── Content ─────────────────────────
        ColumnLayout {
            visible: !window.settingsOpen && !window.helpOpen
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Theme.s16

            DesktopGalleryHeader {
                titleText: {
                    if (app.currentGameAvailable && app.gameId === app.currentGameId && app.category === "favorites")
                        return "Game Favourites"
                    if (app.gameId >= 0) {
                        for (const g of app.games)
                            if (g.id === app.gameId) return g.name
                    }
                    return app.category.charAt(0).toUpperCase() + app.category.slice(1)
                }
                bulkMode: window.bulkMode
                bulkCount: window.bulkCount()
                bulkAllSelected: window.bulkAllSelected()
                onBulkEnterRequested: window.bulkEnter()
                onBulkSelectAllRequested: window.bulkSelectAll()
                onBulkDeleteRequested: window.bulkAskDelete()
                onBulkExitRequested: window.bulkExit()
            }
            DesktopGalleryGrid {
                id: grid
                host: window
                deleteDialog: deleteDialog
                bulkDeleteDialog: bulkDeleteDialog
                onCaptureActivated: (index) => lightbox.openAt(index)
                onDeleteRequested: (index, gameName, dateText) => window.askDelete(index, gameName, dateText)
                onAddFolderRequested: folderDialog.open()
            }

            DesktopGalleryFooter {
                hasCaptures: grid.count > 0
                sidebarFocused: window.sidebarFocused
                bulkMode: window.bulkMode
                usingGamepad: window.usingGamepad
                zoomLevel: window.zoomLevel
                onZoomOutRequested: window.zoomOut()
                onZoomMoved: (value) => window.zoomLevel = value
                onZoomInRequested: window.zoomIn()
            }
        }

    }

    FolderDialog {
        id: folderDialog
        title: "Choose a folder to watch"
        onAccepted: app.addWatchedFolder(selectedFolder)
    }

    // ───────────────────────── Lightbox viewer ─────────────────────────
    Lightbox {
        id: lightbox
        parentWindow: window
        galleryModel: app.gallery
        onClosed: grid.forceActiveFocus()
    }

    // ───────────────────────── Delete confirmation ─────────────────────────
    property int pendingDeleteRow: -1

    function askDelete(row, name, date) {
        window.pendingDeleteRow = row
        deleteDialog.message = name + " · " + date + "\nThis permanently deletes the file."
        deleteDialog.open()
    }

    ConfirmDialog {
        id: deleteDialog
        anchors.fill: parent
        z: 100
        title: "Delete capture?"
        confirmLabel: "Delete"
        onConfirmed: {
            sounds.play("confirm")
            app.deleteCapture(window.pendingDeleteRow)
            window.pendingDeleteRow = -1
        }
        onCanceled: window.pendingDeleteRow = -1
    }

    ConfirmDialog {
        id: bulkDeleteDialog
        anchors.fill: parent
        z: 100
        title: "Delete selected captures?"
        confirmLabel: "Delete"
        onConfirmed: window.bulkConfirmDelete()
    }

    // ───────────────────────── Pad action menu (Square) ─────────────────────────
    // Show in folder / Delete for the currently pad-selected tile — mouse
    // users already have per-tile hover icons for this; pad users need an
    // equivalent that doesn't require hovering.
    OverlayActionMenu {
        id: padMenu
        // Bulk select is desktop-only, so it is added here rather than in the
        // shared component. padMenuConfirm() maps these by index.
        entries: ["Show in folder", "Delete", "Bulk select"]
        open: window.menuOpen
        currentIndex: window.menuIndex
        onCloseRequested: window.menuOpen = false
        onItemHovered: function(index) { window.menuIndex = index }
        onItemConfirmed: function(index) {
            window.menuIndex = index
            window.padMenuConfirm()
        }
    }
}
