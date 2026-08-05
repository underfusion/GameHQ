.pragma library

// Spatial gamepad focus navigation shared by the Settings panels and the
// modal dialogs that open above them. Candidates are the visible, enabled,
// tab-focusable descendants of `scope`; geometry is mapped into `scope`
// coordinates so nested layouts don't matter.

function isInside(item, ancestor) {
    for (let p = item; p; p = p.parent)
        if (p === ancestor)
            return true
    return false
}

// Focus-chain sweep restricted to `scope`. The chain cycles through the whole
// window, so the sweep stops when it revisits an in-scope item.
function focusables(scope) {
    const list = []
    if (!scope)
        return list
    let probe = scope.nextItemInFocusChain(true)
    for (let guard = 0; probe && guard < 500; ++guard) {
        if (isInside(probe, scope) && probe.activeFocusOnTab
                && probe.visible && probe.enabled) {
            if (list.indexOf(probe) >= 0)
                break
            list.push(probe)
        }
        probe = probe.nextItemInFocusChain(true)
    }
    return list
}

// Next focus target below (+1) or above (-1) the active item: nearest row
// first, then the closest column within that row. With no active item inside
// the scope, returns the first focusable so entering a panel lands at the top.
function verticalTarget(scope, active, direction) {
    const items = focusables(scope)
    if (items.length === 0)
        return null
    if (!active || !isInside(active, scope))
        return items[0]
    const a = active.mapToItem(scope, 0, 0)
    const ax = a.x + active.width / 2
    const ay = a.y + active.height / 2
    let best = null
    let bestDy = 0
    let bestDx = 0
    for (let i = 0; i < items.length; ++i) {
        const it = items[i]
        if (it === active)
            continue
        const p = it.mapToItem(scope, 0, 0)
        const dy = direction > 0 ? (p.y + it.height / 2) - ay
                                 : ay - (p.y + it.height / 2)
        if (dy <= 1)
            continue
        const dx = Math.abs((p.x + it.width / 2) - ax)
        if (!best || dy < bestDy - 1
                || (Math.abs(dy - bestDy) <= 1 && dx < bestDx)) {
            best = it
            bestDy = dy
            bestDx = dx
        }
    }
    return best
}

// Focusable whose centre lies nearest to (x, y) in `scope` coordinates — used
// to restore a sensible pad position after the focused control was destroyed
// or disabled by its own action (e.g. Restore defaults reloading the binding
// cards). Vertical distance dominates so restoration stays in the same visual
// row whenever possible.
function nearestTarget(scope, x, y) {
    const items = focusables(scope)
    let best = null
    let bestScore = Infinity
    for (let i = 0; i < items.length; ++i) {
        const it = items[i]
        const p = it.mapToItem(scope, 0, 0)
        const dy = Math.abs(p.y + it.height / 2 - y)
        const dx = Math.abs(p.x + it.width / 2 - x)
        const score = dy * 1000 + dx
        if (score < bestScore) {
            best = it
            bestScore = score
        }
    }
    return best
}

// Next focus target to the right (+1) or left (-1) within the active item's
// row — vertical extents must overlap, so this never jumps across rows.
// Returns null at the row edge; the caller decides what an edge means.
function horizontalTarget(scope, active, direction) {
    if (!active || !isInside(active, scope))
        return null
    const items = focusables(scope)
    const a = active.mapToItem(scope, 0, 0)
    let best = null
    let bestX = 0
    for (let i = 0; i < items.length; ++i) {
        const it = items[i]
        if (it === active)
            continue
        const p = it.mapToItem(scope, 0, 0)
        if (p.y + it.height <= a.y + 1 || p.y >= a.y + active.height - 1)
            continue
        if (direction > 0 ? p.x <= a.x + 1 : p.x >= a.x - 1)
            continue
        if (!best || (direction > 0 ? p.x < bestX : p.x > bestX)) {
            best = it
            bestX = p.x
        }
    }
    return best
}
