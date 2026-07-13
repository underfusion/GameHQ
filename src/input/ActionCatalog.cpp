#include "input/ActionCatalog.h"

namespace {

const QVector<ActionCatalog::Action>& buildCatalog()
{
    using Scope = ActionCatalog::Scope;
    static const QVector<ActionCatalog::Action> actions = {
        // Global — always active, independent of overlay/desktop/playback state.
        { QStringLiteral("global.screenshot"),     Scope::Global,   QStringLiteral("Screenshot"),
          QStringLiteral("Capture a screenshot of the current game."), true },
        { QStringLiteral("global.save_replay"),    Scope::Global,   QStringLiteral("Save Replay"),
          QStringLiteral("Save the rolling replay buffer as a clip."), true },
        { QStringLiteral("global.toggle_buffer"),  Scope::Global,   QStringLiteral("Toggle Replay Buffer"),
          QStringLiteral("Start or stop the rolling replay buffer."), true },
        { QStringLiteral("global.toggle_overlay"), Scope::Global,   QStringLiteral("Toggle Overlay"),
          QStringLiteral("Show or hide the in-game overlay."), true },

        // Overlay — in-game overlay is visible.
        { QStringLiteral("overlay.navigate_up"),    Scope::Overlay, QStringLiteral("Navigate Up"),
          QStringLiteral("Move selection up in the overlay."), true },
        { QStringLiteral("overlay.navigate_down"),  Scope::Overlay, QStringLiteral("Navigate Down"),
          QStringLiteral("Move selection down in the overlay."), true },
        { QStringLiteral("overlay.navigate_left"),  Scope::Overlay, QStringLiteral("Navigate Left"),
          QStringLiteral("Move selection left in the overlay."), true },
        { QStringLiteral("overlay.navigate_right"), Scope::Overlay, QStringLiteral("Navigate Right"),
          QStringLiteral("Move selection right in the overlay."), true },
        { QStringLiteral("overlay.confirm"),        Scope::Overlay, QStringLiteral("Confirm"),
          QStringLiteral("Activate the selected item in the overlay."), true },
        { QStringLiteral("overlay.back"),           Scope::Overlay, QStringLiteral("Back"),
          QStringLiteral("Close the current overlay panel."), false },
        { QStringLiteral("overlay.favorite"),       Scope::Overlay, QStringLiteral("Toggle Favorite"),
          QStringLiteral("Mark or unmark the selected capture as a favorite."), true },
        { QStringLiteral("overlay.menu"),           Scope::Overlay, QStringLiteral("Open Menu"),
          QStringLiteral("Open the action menu for the selected item."), true },
        { QStringLiteral("overlay.sidebar_toggle"), Scope::Overlay, QStringLiteral("Toggle Sidebar"),
          QStringLiteral("Show or hide the overlay sidebar."), true },
        { QStringLiteral("overlay.game_prev"),      Scope::Overlay, QStringLiteral("Previous Game"),
          QStringLiteral("Step to the previous game in the sidebar."), true },
        { QStringLiteral("overlay.game_next"),      Scope::Overlay, QStringLiteral("Next Game"),
          QStringLiteral("Step to the next game in the sidebar."), true },

        // Desktop — the desktop gallery window has real Win32 foreground focus.
        { QStringLiteral("desktop.navigate_up"),    Scope::Desktop, QStringLiteral("Navigate Up"),
          QStringLiteral("Move selection up in the gallery."), true },
        { QStringLiteral("desktop.navigate_down"),  Scope::Desktop, QStringLiteral("Navigate Down"),
          QStringLiteral("Move selection down in the gallery."), true },
        { QStringLiteral("desktop.navigate_left"),  Scope::Desktop, QStringLiteral("Navigate Left"),
          QStringLiteral("Move selection left in the gallery."), true },
        { QStringLiteral("desktop.navigate_right"), Scope::Desktop, QStringLiteral("Navigate Right"),
          QStringLiteral("Move selection right in the gallery."), true },
        { QStringLiteral("desktop.confirm"),        Scope::Desktop, QStringLiteral("Confirm"),
          QStringLiteral("Activate the selected item in the gallery."), true },
        { QStringLiteral("desktop.back"),           Scope::Desktop, QStringLiteral("Back"),
          QStringLiteral("Close the current gallery panel."), false },
        { QStringLiteral("desktop.favorite"),       Scope::Desktop, QStringLiteral("Toggle Favorite"),
          QStringLiteral("Mark or unmark the selected capture as a favorite."), true },
        { QStringLiteral("desktop.menu"),           Scope::Desktop, QStringLiteral("Open Menu"),
          QStringLiteral("Open the action menu for the selected item."), true },
        { QStringLiteral("desktop.tab_prev"),       Scope::Desktop, QStringLiteral("Previous Tab"),
          QStringLiteral("Step to the previous sidebar category."), true },
        { QStringLiteral("desktop.tab_next"),       Scope::Desktop, QStringLiteral("Next Tab"),
          QStringLiteral("Step to the next sidebar category."), true },

        // Playback — a clip is focused/playing in the overlay or gallery lightbox.
        { QStringLiteral("playback.play_pause"),  Scope::Playback, QStringLiteral("Play / Pause"),
          QStringLiteral("Toggle playback of the focused clip."), true },
        { QStringLiteral("playback.seek_back"),   Scope::Playback, QStringLiteral("Seek Back"),
          QStringLiteral("Step the focused clip backward."), true },
        { QStringLiteral("playback.seek_forward"),Scope::Playback, QStringLiteral("Seek Forward"),
          QStringLiteral("Step the focused clip forward."), true },
    };
    return actions;
}

} // namespace

const QVector<ActionCatalog::Action>& ActionCatalog::all()
{
    return buildCatalog();
}

const ActionCatalog::Action* ActionCatalog::find(const QString& id)
{
    const auto& actions = all();
    for (const auto& action : actions) {
        if (action.id == id)
            return &action;
    }
    return nullptr;
}
