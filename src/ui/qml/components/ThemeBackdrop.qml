import QtQuick
import QtQuick.Effects
import GameHQ

// Paints the desktop window behind its content, per the active skin's
// `backdropStyle`. "flat" reproduces the original single-color fill exactly, so
// the default Dark skin looks untouched.
//
// Everything here is decorative and must never intercept input — the whole item
// is below the content and takes no mouse events.
Item {
    id: root

    readonly property string style: Theme.backdropStyle

    // Base fill: always present, so any layer above can be partial.
    Rectangle {
        anchors.fill: parent
        color: Theme.bg0
    }

    // Vertical wash. `flat` skips it entirely rather than painting a gradient
    // between two identical colors.
    Rectangle {
        anchors.fill: parent
        visible: root.style !== "flat"
        gradient: Gradient {
            GradientStop { position: 0.0; color: Theme.backdropTop }
            GradientStop { position: 1.0; color: Theme.backdropBottom }
        }
    }

    // Soft accent orbs, blurred into washes. Two off-canvas circles bled through
    // a heavy blur — cheap, static, and it reads like the light behind a console
    // dashboard rather than a drawn shape.
    Item {
        id: orbs
        anchors.fill: parent
        visible: root.style === "wash"
        layer.enabled: visible
        layer.effect: MultiEffect {
            blurEnabled: true
            blur: 1.0
            blurMax: 64
        }

        Rectangle {
            width: parent.width * 0.9
            height: width
            radius: width / 2
            x: -parent.width * 0.25
            y: -parent.height * 0.45
            color: Theme.washA
            opacity: 0.22 * Theme.glowStrength
        }
        Rectangle {
            width: parent.width * 0.8
            height: width
            radius: width / 2
            x: parent.width * 0.5
            y: parent.height * 0.55
            color: Theme.washB
            opacity: 0.18 * Theme.glowStrength
        }
    }

    // Fine horizontal rules. Drawn once to a Canvas instead of a Repeater of
    // hundreds of Rectangles — this is a static backdrop, not a live effect.
    Canvas {
        id: scanlines
        anchors.fill: parent
        visible: root.style === "scanlines"
        opacity: 0.5
        onPaint: {
            const ctx = getContext("2d")
            ctx.reset()
            ctx.strokeStyle = Qt.rgba(1, 1, 1, 0.05 + 0.05 * Theme.glowStrength)
            ctx.lineWidth = 1
            for (let y = 0.5; y < height; y += 3) {
                ctx.beginPath()
                ctx.moveTo(0, y)
                ctx.lineTo(width, y)
                ctx.stroke()
            }
        }
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
        Connections {
            target: Theme
            function onSkinChanged() { scanlines.requestPaint() }
        }
    }
}
