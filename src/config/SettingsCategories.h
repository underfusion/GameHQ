#pragma once
#include "config/ConfigKeys.h"

#include <QHash>
#include <QString>
#include <QStringList>

// Maps a settings page ("Restore defaults" on that page) to the config groups it
// owns. Kept next to ConfigKeys so the page taxonomy and the key spellings stay
// reviewable side by side.
//
// Group entries cover whole prefixes owned by a page. keys() records individual
// controls whose prefixes are shared with another page. Input bindings have
// their own database-backed restore path. Library has no config group of its
// own (watched folders are DB rows).
namespace SettingsCategories
{
inline const QHash<QString, QStringList>& groups()
{
    static const QHash<QString, QStringList> kGroups = {
        { QStringLiteral("General"),              { ConfigKeys::Group::Startup,
                                                    ConfigKeys::Group::Tray,
                                                    ConfigKeys::Group::Theme } },
        { QStringLiteral("Capture"),              { ConfigKeys::Group::Capture } },
        { QStringLiteral("Replay"),               { ConfigKeys::Group::Replay,
                                                    ConfigKeys::Group::Audio } },
        { QStringLiteral("Notifications & Sound"), { ConfigKeys::Group::Sounds,
                                                     ConfigKeys::Group::Notifications } },
        { QStringLiteral("About"),                { ConfigKeys::Group::Updates } },
    };
    return kGroups;
}

// Individual keys shown on a page without giving that page ownership of the
// key's entire prefix.
inline const QHash<QString, QStringList>& keys()
{
    static const QHash<QString, QStringList> kKeys = {
        { QStringLiteral("Capture"), {
            ConfigKeys::StorageScreenshotsRoot,
            ConfigKeys::StorageClipsRoot,
        } },
        { QStringLiteral("Notifications & Sound"), {
            ConfigKeys::CaptureScreenshotSound,
            ConfigKeys::CaptureScreenshotNotify,
            ConfigKeys::ReplayClipSound,
            ConfigKeys::ReplayClipNotify,
        } },
    };
    return kKeys;
}
} // namespace SettingsCategories
