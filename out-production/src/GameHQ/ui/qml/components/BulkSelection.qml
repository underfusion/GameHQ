import QtQuick

// The bulk-selection state machine, lifted out of Main.qml so the selection
// algebra can be read on its own: which paths are picked, where the range
// anchor sits, and how a shift-extend rebuilds the set from the base snapshot.
//
// Deliberately pure: model queries and state only. Sounds, focus moves, dialogs
// and the pad flow stay in Main.qml, which was hand-verified against a real
// DualSense — this file must not be the reason that behavior shifts.
QtObject {
    id: root

    // The gallery model selections are computed against (app.gallery).
    property var galleryModel

    // filePath -> true for every picked capture. Reassigned rather than mutated
    // so QML bindings on it re-evaluate.
    property var selected: ({})

    // Where a shift-extend measures from; -1 once the anchor is meaningless.
    property int anchorIndex: -1

    // A shift-extend in progress replays against rangeBase (the selection as it
    // stood when the extend began) so dragging back over the range undoes it
    // instead of leaving a trail.
    property bool rangeActive: false
    property bool rangeSelecting: true
    property var rangeBase: ({})

    function count() {
        var n = 0
        for (var path in root.selected)
            if (root.selected[path])
                ++n
        return n
    }

    function isChecked(path) {
        return !!root.selected[path]
    }

    function allSelected() {
        return root.galleryModel.rowCount() > 0
            && root.count() === root.galleryModel.rowCount()
    }

    function clear() {
        root.selected = ({})
        root.anchorIndex = -1
        root.rangeActive = false
        root.rangeSelecting = true
        root.rangeBase = ({})
    }

    // Returns true when the selection actually changed, so the caller knows
    // whether to play the tick.
    function toggle(index, extendRange) {
        if (index < 0 || index >= root.galleryModel.rowCount())
            return false

        var record = root.galleryModel.get(index)
        var path = record.filePath
        if (!path)
            return false

        if (extendRange && root.anchorIndex >= 0
                && root.anchorIndex < root.galleryModel.rowCount()) {
            if (!root.rangeActive) {
                var base = {}
                for (var basePath in root.selected)
                    if (root.selected[basePath])
                        base[basePath] = true
                root.rangeBase = base
                root.rangeSelecting = !root.selected[path]
                root.rangeActive = true
            }

            var first = Math.min(root.anchorIndex, index)
            var last = Math.max(root.anchorIndex, index)
            var rangePaths = {}
            for (var i = first; i <= last; ++i) {
                var rangeRecord = root.galleryModel.get(i)
                if (!rangeRecord.filePath)
                    continue
                rangePaths[rangeRecord.filePath] = true
            }

            var copy = {}
            for (var key in root.rangeBase)
                if (root.rangeBase[key]
                        && (root.rangeSelecting || !rangePaths[key]))
                    copy[key] = true
            if (root.rangeSelecting)
                for (var rangePath in rangePaths)
                    copy[rangePath] = true

            root.selected = copy
        } else {
            var toggled = {}
            for (var selectedPath in root.selected)
                if (selectedPath !== path && root.selected[selectedPath])
                    toggled[selectedPath] = true
            if (!root.selected[path])
                toggled[path] = true
            root.selected = toggled
            root.anchorIndex = index
            root.rangeActive = false
            root.rangeSelecting = true
            root.rangeBase = ({})
        }
        return true
    }

    // Select-all doubles as deselect-all once everything is picked. Returns true
    // when it cleared, false when it selected, so the caller picks the sound.
    function selectAll() {
        if (root.allSelected()) {
            root.clear()
            return true
        }
        var copy = {}
        for (var i = 0; i < root.galleryModel.rowCount(); ++i) {
            var rec = root.galleryModel.get(i)
            if (rec.filePath)
                copy[rec.filePath] = true
        }
        root.selected = copy
        root.anchorIndex = -1
        root.rangeActive = false
        root.rangeSelecting = true
        root.rangeBase = ({})
        return false
    }

    // Model row indices of the current selection, in model order.
    function rows() {
        var out = []
        for (var i = 0; i < root.galleryModel.rowCount(); ++i) {
            var rec = root.galleryModel.get(i)
            if (rec.filePath && root.selected[rec.filePath])
                out.push(i)
        }
        return out
    }
}
