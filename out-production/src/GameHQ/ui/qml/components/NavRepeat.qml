import QtQuick

// Hold-to-repeat wrapper for navigation actions.
//
// Used by Main.qml and OverlayWindow.qml to give keyboard arrows/WASD the
// same "press once → 0.5 s delay → gradually accelerate" feel as the pad
// (whose repeat lives in InputEngine). QML's Keys.onPressed sees the OS's
// auto-repeat events directly, so callers MUST ignore `event.isAutoRepeat`
// in their handler and call NavRepeat.start() on the real keypress instead;
// NavRepeat.stop() goes in Keys.onReleased.
//
//   initialDelayMs   — pause before auto-repeat begins (default 500 ms)
//   startIntervalMs  — interval of the first auto-repeat tick (default 220 ms)
//   acceleration     — multiplier applied per tick (default 0.77 = 0.88²,
//                      2x the shrink-per-tick of the original 0.88; smaller =
//                      faster ramp). The interval never drops below minIntervalMs.
//   action           — () => void, fired once on start() then on each auto-repeat
//
// Only one direction is held at a time, so a single instance per surface is
// enough: reassigning `action` and calling start() atomically swaps targets.
Item {
    id: root

    property int initialDelayMs: 500
    property int startIntervalMs: 220
    property int minIntervalMs: 70
    property real acceleration: 0.77
    property var action: null
    // Optional callback fired on EVERY tick (immediate + each auto-repeat),
    // before `action`. Callers use it to refresh UI locks that must stay held
    // for the whole repeat (e.g. suppress hover-follow while keys are held).
    property var onTick: null

    // Internal timers — declared as children so they parent to root.
    Timer {
        id: initialTimer
        interval: root.initialDelayMs
        repeat: false
        onTriggered: {
            tickTimer.interval = root.startIntervalMs
            tickTimer.start()
        }
    }

    Timer {
        id: tickTimer
        interval: root.startIntervalMs
        repeat: true
        onTriggered: {
            if (root.onTick)
                root.onTick()
            if (root.action)
                root.action()
            // Accelerate: shrink interval toward the floor.
            tickTimer.interval = Math.max(
                root.minIntervalMs,
                Math.floor(tickTimer.interval * root.acceleration))
        }
    }

    // Fire the action once immediately, then arm the initial-delay timer.
    // Calling start() while a repeat is already running atomically replaces
    // it (e.g. switching from LEFT to RIGHT without releasing).
    function start() {
        stop()
        if (root.onTick)
            root.onTick()
        if (root.action)
            root.action()
        initialTimer.interval = root.initialDelayMs
        tickTimer.interval = root.startIntervalMs
        initialTimer.start()
    }

    function stop() {
        initialTimer.stop()
        tickTimer.stop()
    }
}
