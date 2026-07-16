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

    // Procedural texture, tiled.
    //
    // The tile is generated once at 96x96 and repeated across the window rather
    // than drawn at full size: a full-window Canvas would cost ~33 MB of pixels
    // at 4K and repaint on every resize, while a tile costs ~36 KB and never
    // repaints. Tiling also means the pattern stays pixel-exact at any size or
    // DPI — the reason this is generated instead of a bundled image.
    //
    // Every pattern is authored to tile seamlessly: spacings divide 96 evenly
    // and the diagonals wrap at the tile edge.
    Canvas {
        id: textureTile
        width: 96
        height: 96
        visible: false
        readonly property color ink: Theme.textureColor

        onPaint: {
            const ctx = getContext("2d")
            ctx.reset()
            ctx.clearRect(0, 0, width, height)
            const style = Theme.texture
            ctx.strokeStyle = textureTile.ink
            ctx.fillStyle = textureTile.ink
            ctx.lineWidth = 1

            if (style === "grain") {
                // Deterministic specks: a fixed pseudo-random walk, so the tile
                // is identical every regeneration and cannot shimmer.
                let seed = 1337
                const rand = function () {
                    seed = (seed * 1103515245 + 12345) & 0x7fffffff
                    return seed / 0x7fffffff
                }
                for (let i = 0; i < 900; ++i) {
                    ctx.globalAlpha = 0.25 + rand() * 0.75
                    ctx.fillRect(Math.floor(rand() * width), Math.floor(rand() * height), 1, 1)
                }
            } else if (style === "hatch") {
                // 45° rules drawn past both edges so the wrap is seamless.
                for (let x = -height; x < width; x += 8) {
                    ctx.beginPath()
                    ctx.moveTo(x, height)
                    ctx.lineTo(x + height, 0)
                    ctx.stroke()
                }
            } else if (style === "grid") {
                for (let g = 0.5; g < width; g += 24) {
                    ctx.beginPath(); ctx.moveTo(g, 0); ctx.lineTo(g, height); ctx.stroke()
                    ctx.beginPath(); ctx.moveTo(0, g); ctx.lineTo(width, g); ctx.stroke()
                }
            } else if (style === "weave") {
                // Twill: short strokes alternating direction per cell, which
                // reads as an over-under weave once tiled.
                const cell = 12
                for (let y = 0; y < height; y += cell) {
                    for (let x = 0; x < width; x += cell) {
                        const flip = ((x / cell) + (y / cell)) % 2 === 0
                        ctx.beginPath()
                        if (flip) { ctx.moveTo(x, y); ctx.lineTo(x + cell, y + cell) }
                        else       { ctx.moveTo(x + cell, y); ctx.lineTo(x, y + cell) }
                        ctx.stroke()
                    }
                }
            }
        }
        // toDataURL() forces a synchronous render that re-emits painted();
        // without this latch the handler re-enters itself until the JS stack
        // blows and the process dies on startup for any skin with a texture.
        property bool exported: false
        onPainted: {
            if (exported)
                return
            exported = true
            textureLayer.source = toDataURL()
        }
    }

    Image {
        id: textureLayer
        anchors.fill: parent
        visible: Theme.texture !== "none"
        fillMode: Image.Tile
        opacity: Theme.textureOpacity
        smooth: false          // a texel is a texel; smoothing would blur the grain
        Component.onCompleted: textureTile.requestPaint()
        Connections {
            target: Theme
            function onSkinChanged() {
                textureTile.exported = false
                textureTile.requestPaint()
            }
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
