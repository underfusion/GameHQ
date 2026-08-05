#include "input/ContextOverrideCatalog.h"

const QVector<ContextOverrideCatalog::Override>& ContextOverrideCatalog::all()
{
    static const QVector<Override> catalog = {
        // Share (Create) tap while a clip is focused grabs the on-screen frame
        // *instead of* taking a desktop screenshot — one button press must not
        // produce two captures. Scoped to the tap gesture only: the same button
        // held is global.save_replay and double-tapped is global.toggle_overlay,
        // and both stay active during playback.
        {QStringLiteral("playback.frame_grab"),
         QStringLiteral("global.screenshot"),
         GestureSpec::tap(1)},
    };
    return catalog;
}

bool ContextOverrideCatalog::shadows(const QString& overridingActionId,
                                     const QString& shadowedActionId,
                                     const GestureSpec& gesture)
{
    for (const Override& entry : all()) {
        if (entry.overridingActionId == overridingActionId
            && entry.shadowedActionId == shadowedActionId
            && entry.gesture.kind == gesture.kind
            && entry.gesture.tapCount == gesture.tapCount) {
            return true;
        }
    }
    return false;
}
