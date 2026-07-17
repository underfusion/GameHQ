import QtQuick
import QtMultimedia
import GameHQ

// Double-buffered still/clip stage shared by Lightbox.qml and OverlayPreview.qml.
//
// The double buffer: `loader` async-decodes `targetUrl` off the UI thread and
// promotes it to `committedUrl` — the source of the visible `still` — only once
// it reaches Image.Ready. The previously committed still stays painted until
// then, so stepping between captures never flashes an empty stage.
//
// The two callers deliberately disagree about what sits behind a clip: the
// Lightbox paints nothing there, while the overlay keeps the clip's thumbnail up
// until playback is focused. Every rule that differs is therefore set by the
// caller; this component owns only what is identical between them — the
// decode-then-promote handoff and the player/end-of-media wiring.
Item {
    id: root

    // What the hidden loader decodes. "" means "nothing to decode".
    property url targetUrl: ""
    // Whether the committed still paints (callers hide it behind a clip).
    property bool stillVisible: true
    // What the player plays. "" means "no clip selected".
    property url videoSource: ""
    property bool videoVisible: false

    // Clear the committed still when targetUrl goes empty. OFF for the
    // Lightbox on purpose: it blanks targetUrl for every clip, and dropping the
    // committed still there would make the next image step decode against an
    // empty stage — the flash the double buffer exists to prevent.
    property bool clearOnEmptyTarget: false
    // Stop the player when its source clears. The overlay leaves the player
    // alone instead and re-sources it when playback is focused.
    property bool stopOnEmptySource: false

    // The already-decoded still on screen. Writable so a caller can commit a
    // capture eagerly (Lightbox.openAt) or drop it on close.
    property url committedUrl: ""

    readonly property alias player: clipPlayer
    readonly property real sourceWidth: still.implicitWidth
    readonly property real sourceHeight: still.implicitHeight

    // A freshly decoded still was just promoted.
    signal committed(url source)
    // targetUrl went empty and the stage has nothing left to paint.
    signal cleared()
    // Playback lifecycle — callers drive their own PlayerControls from these.
    signal playbackStarted()
    signal playbackEnded()

    Image {
        id: still
        anchors.fill: parent
        visible: root.stillVisible
        source: root.committedUrl
        fillMode: Image.PreserveAspectFit
        cache: true
    }

    Image {
        id: loader
        anchors.fill: parent
        visible: false
        source: root.targetUrl
        asynchronous: true
        cache: true
        onSourceChanged: {
            if (source.toString() === "" && root.clearOnEmptyTarget) {
                root.committedUrl = ""
                root.cleared()
            }
        }
        onStatusChanged: {
            if (status === Image.Ready && source.toString() !== "") {
                root.committedUrl = source
                root.committed(source)
            }
        }
    }

    // QML Image can't decode video — clips play through the player rather than
    // failing to decode the raw .mp4 as a still frame.
    MediaPlayer {
        id: clipPlayer
        source: root.videoSource
        audioOutput: AudioOutput {}
        videoOutput: videoSurface
        onSourceChanged: {
            if (source.toString() !== "") {
                play()
                root.playbackStarted()
            } else if (root.stopOnEmptySource) {
                stop()
            }
        }
        onMediaStatusChanged: {
            if (mediaStatus === MediaPlayer.EndOfMedia) {
                pause()
                if (duration > 0)
                    position = duration
                root.playbackEnded()
            }
        }
    }

    VideoOutput {
        id: videoSurface
        anchors.fill: parent
        visible: root.videoVisible
        fillMode: VideoOutput.PreserveAspectFit
    }
}
