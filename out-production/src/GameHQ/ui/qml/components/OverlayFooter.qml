import QtQuick
import GameHQ

Text {
    id: root

    property bool usingGamepad: false
    property bool menuOpen: false
    property bool videoFocused: false

    anchors.bottom: parent.bottom
    anchors.horizontalCenter: parent.horizontalCenter

    text: {
        const pad = root.usingGamepad
        if (root.menuOpen)
            return pad
                ? "D-pad Up/Down - choose | Cross - confirm | Circle - close menu"
                : "Up/Down - choose | Enter - confirm | Esc/Backspace - close menu"
        if (root.videoFocused)
            return pad
                ? "D-pad Left/Right - scrub | Cross - play/pause | Circle - back to captures"
                : "Left/Right - scrub clip | Enter - play/pause | Esc/Backspace - back to captures"
        return pad
            ? "L1/R1 - captures | D-pad Up/Down - categories/games | Cross - open | Triangle - favorite | Square - menu | Circle - back to game"
            : "Left/Right - captures | Up/Down - categories/games | Enter - open | F - favorite | M - menu | Esc - back to game"
    }
    color: Theme.textMuted
    font.family: Theme.fontFamily
    font.pixelSize: Theme.fontCaption
}
