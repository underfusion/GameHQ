#pragma once
#include "config/ConfigKeys.h"

#include <QHash>
#include <QString>
#include <QStringList>

// Maps a settings page ("Restore defaults" on that page) to the config groups it
// owns. Kept next to ConfigKeys so the page taxonomy and the key spellings stay
// reviewable side by side.
//
// Each entry lists every config prefix that page actually reads/writes. Input
// bindings have their own database-backed restore path. Library has no config
// group of its own (watched folders are DB rows). The screenshot/clip folder
// pickers live on the Capture page even though the clip root also feeds Replay,
// so both are reset as single keys — see CaptureOnlyKeys — rather than via the
// shared "storage" group, which would silently affect both pages.
namespace SettingsCategories
{
inline const QHash<QString, QStringList>& groups()
{
    static const QHash<QString, QStringList> kGroups = {
        { QStringLiteral("General"),              { ConfigKeys::Group::Startup,
                                                    ConfigKeys::Group::Tray } },
        { QStringLiteral("Capture"),              { ConfigKeys::Group::Capture } },
        { QStringLiteral("Replay"),               { ConfigKeys::Group::Replay,
                                                    ConfigKeys::Group::Audio } },
        { QStringLiteral("Notifications & Sound"), { ConfigKeys::Group::Sounds,
                                                     ConfigKeys::Group::Notifications } },
        { QStringLiteral("About"),                { ConfigKeys::Group::Updates } },
    };
    return kGroups;
}

// The page that owns the individual storage.* keys, and the keys it resets.
inline constexpr QLatin1StringView CaptureCategory{ "Capture" };
inline const QStringList& captureOnlyKeys()
{
    static const QStringList kKeys = { ConfigKeys::StorageScreenshotsRoot,
                                       ConfigKeys::StorageClipsRoot };
    return kKeys;
}
} // namespace SettingsCategories
