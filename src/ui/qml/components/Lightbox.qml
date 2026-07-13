import QtQuick
import QtQuick.Window
import QtMultimedia
import GameHQ
import "."

// Full-screen image viewer, styled after OverlayWindow.qml: a separate
// top-level frameless window (NOT the main app window resized) that covers
// the whole monitor with a dimmed scrim and shows the current capture at
// ~80% of that screen. Left/Right cycles through the gallery model, Esc —
// or a click on the dimmed area — closes. Opened from a single left click
// on a CaptureTile (see Main.qml).
Window {
    id: root
    visible: false
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
    color: "transparent"
    title: Brand.name + " Viewer"

    property var galleryModel
    property var parentWindow
    property int index: -1
    // Tracks which input source last navigated the gallery so the side
    // hint pills can render the matching label (L1/R1 for gamepad,
    // ←/→ for keyboard/mouse). Set from padStep() and Keys handlers.
    property bool usingGamepad: false
    readonly property var current: root.index >= 0 && root.galleryModel
        ? root.galleryModel.get(root.index) : ({})

    // ── Double-buffer for instant capture switching ────────────────────
    // The visible Image shows `_committedUrl` — always an already-decoded
    // image, so it never goes blank between captures. The hidden `loader`
    // async-decodes `_targetUrl` (the just-navigated-to capture's URL);
    // only when it reaches Image.Ready is its source promoted to
    // `_committedUrl`. Result: stepping with L1/R1 or ←/→ keeps the
    // previous capture painted on screen until the next one is ready,
    // eliminating the brief dim-scrim flash that previously appeared.
    property url _committedUrl: ""
    readonly property url _targetUrl:
        root.current.captureType !== "video" && root.current.fileUrl
            ? root.current.fileUrl : ""

    screen: root.parentWindow ? root.parentWindow.screen : undefined

    signal closed()

    function openAt(row) {
        root.index = row
        // Commit the opening capture immediately so the first paint isn't
        // a blank stage waiting for the async loader. Subsequent steps
        // leave _committedUrl alone — the loader updates it when Ready.
        root._committedUrl = root._targetUrl
        root.showFullScreen()
        root.requestActivate()
        content.forceActiveFocus()
    }
    function close() {
        clipPlayer.stop()
        root.visible = false
        root.index = -1
        root._committedUrl = ""
        root.closed()
    }
    function step(delta) {
        const count = root.galleryModel ? root.galleryModel.rowCount() : 0
        if (count <= 0)
            return
        root.index = (root.index + delta + count) % count
    }
    function clickStep(delta) {
        root.usingGamepad = false
        root.step(delta)
    }
    function seekVideo(deltaMs) {
        if (!clipPlayer || clipPlayer.duration <= 0)
            return
        playerControls.revealControls()
        clipPlayer.position = Math.max(0, Math.min(clipPlayer.duration, clipPlayer.position + deltaMs))
    }
    function toggleVideoPlayback() {
        if (root.current.captureType !== "video")
            return
        playerControls.revealControls()
        if (clipPlayer.playbackState === MediaPlayer.PlayingState) {
            clipPlayer.pause()
            playerControls.showPulse(false)
        } else {
            clipPlayer.play()
            playerControls.showPulse(true)
        }
    }
    function padStep(direction) {
        // L1/R1 from the pad: step through every capture (images AND
        // videos) so the bumper acts like a "next/prev item" shortcut,
        // regardless of what's currently shown. Videos keep their seek
        // bar reachable via d-pad left/right (see padNavigate).
        root.usingGamepad = true
        root.step(direction)
    }
    function padNavigate(direction) {
        if (root.current.captureType === "video") {
            root.seekVideo(direction * playerControls.seekStepMs)
        } else {
            root.usingGamepad = true
            root.step(direction)
        }
    }
    function padConfirm() {
        if (root.current.captureType === "video")
            root.toggleVideoPlayback()
    }
    function padReveal() {
        if (root.current.captureType === "video")
            playerControls.revealControls()
    }

    Item {
        id: content
        anchors.fill: parent
        focus: true

        Keys.onPressed: (event) => {
            if (root.current.captureType === "video")
                playerControls.revealControls()
            root.usingGamepad = false
            event.accepted = input.handleKeyPressed(event.key, event.modifiers,
                                                    event.isAutoRepeat)
        }
        Keys.onReleased: (event) => {
            event.accepted = input.handleKeyReleased(event.key, event.modifiers)
        }

        // Dimmed background — click anywhere outside the image to close.
        Rectangle {
            anchors.fill: parent
            color: Theme.lightboxScrim
            MouseArea { anchors.fill: parent; onClicked: root.close() }
        }

        Item {
            id: stage
            width: content.width * 0.8
            height: content.height * 0.8
            anchors.centerIn: parent

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                onClicked: (mouse) => {
                    if (mouse.button === Qt.RightButton)
                        root.close()
                }
            }   // swallow left clicks so the scrim doesn't close

            // Visible image — paints `_committedUrl`, which is always an
            // already-decoded image (either set on openAt or promoted from
            // the loader below). Source changes here are cache hits, so the
            // swap is instant — the previous capture stays painted while
            // the next one decodes, eliminating the dim-scrim flash.
            Image {
                anchors.fill: parent
                visible: root.current.captureType !== "video"
                source: root._committedUrl.toString() !== "" ? root._committedUrl : ""
                fillMode: Image.PreserveAspectFit
                cache: true
            }

            // Hidden async loader — decodes `_targetUrl` off the UI thread
            // and promotes it to `_committedUrl` (→ the visible Image
            // above) only when it reaches Image.Ready, so the stage never
            // shows an empty frame between captures.
            Image {
                id: loader
                anchors.fill: parent
                visible: false
                source: root._targetUrl
                asynchronous: true
                cache: true
                onStatusChanged: {
                    if (status === Image.Ready && source.toString() !== "")
                        root._committedUrl = source
                }
            }

            // QML Image can't decode video — clips now play in-app instead of
            // just failing to decode the raw .mp4 as a still frame.
            MediaPlayer {
                id: clipPlayer
                source: (root.current.captureType === "video" && root.current.fileUrl)
                    ? root.current.fileUrl : ""
                audioOutput: AudioOutput {}
                videoOutput: clipVideoOutput
                onSourceChanged: {
                    if (source.toString() !== "") {
                        play()
                        playerControls.showPulse(true)
                    } else {
                        stop()
                    }
                }
                onMediaStatusChanged: {
                    if (mediaStatus === MediaPlayer.EndOfMedia) {
                        pause()
                        if (duration > 0)
                            position = duration
                        playerControls.revealControls()
                    }
                }
            }

            VideoOutput {
                id: clipVideoOutput
                anchors.fill: parent
                visible: root.current.captureType === "video"
                fillMode: VideoOutput.PreserveAspectFit
            }

            PlayerControls {
                id: playerControls
                anchors.fill: parent
                active: root.current.captureType === "video"
                player: clipPlayer
                onPlayPauseRequested: root.toggleVideoPlayback()
                onSeekRequested: (deltaMs) => root.seekVideo(deltaMs)
                onSurfaceRightClicked: root.close()
            }

            Text {
                anchors.bottom: parent.bottom
                anchors.bottomMargin: -Theme.s24
                anchors.horizontalCenter: parent.horizontalCenter
                text: (root.current.gameName || "") + " · " + (root.current.dateText || "")
                color: Theme.textMuted
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontCaption
            }

            // Side hint pills — show which key navigates between captures.
            // Visible for any loaded item (image or video), positioned at
            // the left/right edges of the preview, vertically centered.
            // Label adapts to the last input source: "L1"/"R1" after a pad
            // press, "←"/"→" after a keyboard arrow so the hint matches
            // whatever the user is actually using.
            Rectangle {
                id: closePill
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.topMargin: -Theme.s32
                anchors.rightMargin: -Theme.s32
                width: Theme.s48
                height: Theme.s48
                radius: Theme.radiusPill
                color: Theme.text
                visible: root.index >= 0
                opacity: closeMouse.containsMouse ? 1.0 : 0.92

                Rectangle {
                    width: Theme.s24
                    height: Theme.s4
                    radius: Theme.radiusPill
                    color: Theme.bg0
                    anchors.centerIn: parent
                    rotation: 45
                }
                Rectangle {
                    width: Theme.s24
                    height: Theme.s4
                    radius: Theme.radiusPill
                    color: Theme.bg0
                    anchors.centerIn: parent
                    rotation: -45
                }
                MouseArea {
                    id: closeMouse
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    cursorShape: Qt.PointingHandCursor
                    hoverEnabled: true
                    onClicked: root.close()
                }
                Behavior on opacity { NumberAnimation { duration: Theme.durFast } }
            }

            Rectangle {
                id: pillLeft
                anchors.left: parent.left
                anchors.leftMargin: -Theme.s32
                anchors.verticalCenter: parent.verticalCenter
                width: hintTextL.implicitWidth + Theme.s24
                height: Theme.s48
                radius: Theme.radiusPill
                color: Theme.text
                visible: root.index >= 0
                opacity: leftMouse.containsMouse ? 1.0 : 0.92
                Text {
                    id: hintTextL
                    anchors.centerIn: parent
                    text: root.usingGamepad ? "L1" : "\u2190"
                    color: Theme.bg0
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontTitle
                    font.bold: true
                }
                MouseArea {
                    id: leftMouse
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    cursorShape: Qt.PointingHandCursor
                    hoverEnabled: true
                    onClicked: root.clickStep(-1)
                }
                Behavior on opacity { NumberAnimation { duration: Theme.durFast } }
            }

            Rectangle {
                id: pillRight
                anchors.right: parent.right
                anchors.rightMargin: -Theme.s32
                anchors.verticalCenter: parent.verticalCenter
                width: hintTextR.implicitWidth + Theme.s24
                height: Theme.s48
                radius: Theme.radiusPill
                color: Theme.text
                visible: root.index >= 0
                opacity: rightMouse.containsMouse ? 1.0 : 0.92
                Text {
                    id: hintTextR
                    anchors.centerIn: parent
                    text: root.usingGamepad ? "R1" : "\u2192"
                    color: Theme.bg0
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontTitle
                    font.bold: true
                }
                MouseArea {
                    id: rightMouse
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    cursorShape: Qt.PointingHandCursor
                    hoverEnabled: true
                    onClicked: root.clickStep(1)
                }
                Behavior on opacity { NumberAnimation { duration: Theme.durFast } }
            }
        }
    }
}
