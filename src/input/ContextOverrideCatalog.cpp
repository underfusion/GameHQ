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
         QStringLiteral("tap")},
    };
    return catalog;
}

bool ContextOverrideCatalog::shadows(const QString& overridingActionId,
                                     const QString& shadowedActionId,
                                     const QString& activation)
{
    for (const Override& entry : all()) {
        if (entry.overridingActionId == overridingActionId
            && entry.shadowedActionId == shadowedActionId
            && entry.activation == activation) {
            return true;
        }
    }
    return false;
}
