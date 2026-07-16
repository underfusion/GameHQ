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

// Maps a sidebar category key to the (category, gameId) filter it stands for.
// The two current-game rows are the only special cases: they narrow a normal
// category to the focused game instead of the whole library. gameId -1 means
// "every game".
//
// Callers apply the result on their own surface \u2014 the desktop goes through
// AppController (setGameCategory), the overlay sets its gallery filter
// directly \u2014 but both end at GalleryModel::setFilter with this same pair, which
// is why the mapping belongs here rather than in each sidebar.
function resolveFilter(key, currentGameId) {
    if (key === "game")
        return { category: "all", gameId: currentGameId }
    if (key === "game_favorites")
        return { category: "favorites", gameId: currentGameId }
    return { category: key, gameId: -1 }
}
