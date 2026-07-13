.pragma library

function categories(showCurrentGame) {
    const base = [
        { key: "all",         label: "All",         glyph: "\u25a6" },
        { key: "recent",      label: "Recent",      glyph: "\u25f7" },
        { key: "favorites",   label: "Favorites",   glyph: "\u2661" },
        { key: "screenshots", label: "Screenshots", glyph: "\u25a3" },
        { key: "clips",       label: "Clips",       glyph: "\u25b6" }
    ]

    if (!showCurrentGame)
        return base

    return [
        { key: "game", label: "Game", glyph: "\u25a6" },
        { key: "game_favorites", label: "Game Favourites", glyph: "\u2661" }
    ].concat(base)
}
