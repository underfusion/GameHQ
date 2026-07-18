#include "ui/SettingsRouter.h"

#include "app/StartupManager.h"
#include "config/CaptureLocations.h"
#include "config/ConfigKeys.h"

#include <QDebug>

namespace
{
// The two capture-root keys map 1:1 onto CaptureLocations::Kind.
bool captureRootKind(const QString& key, CaptureLocations::Kind& kind)
{
    if (key == ConfigKeys::StorageScreenshotsRoot) {
        kind = CaptureLocations::Kind::Screenshots;
        return true;
    }
    if (key == ConfigKeys::StorageClipsRoot) {
        kind = CaptureLocations::Kind::Clips;
        return true;
    }
    return false;
}
} // namespace

SettingsRouter::SettingsRouter(StartupManager* startup, CaptureLocations* locations)
    : m_startup(startup)
    , m_locations(locations)
{
}

SettingsRouter::Outcome SettingsRouter::change(const QString& key, const QVariant& value)
{
    // Windows may refuse the run-key write; only persist the toggle if it took.
    if (key == ConfigKeys::StartupEnabled) {
        if (!m_startup->setEnabled(value.toBool())) {
            qWarning() << "Startup: setting change was rejected";
            return Outcome::Rejected;
        }
        return Outcome::Plain;
    }

    CaptureLocations::Kind kind;
    if (captureRootKind(key, kind)) {
        QString error;
        if (!m_locations->setBaseRoot(kind, value.toString(), &error)) {
            qWarning() << "Capture location:" << error;
            return Outcome::Consumed;
        }
        return Outcome::Rescan;
    }

    return Outcome::Plain;
}

SettingsRouter::Outcome SettingsRouter::reset(const QString& key)
{
    if (key == ConfigKeys::StartupEnabled) {
        if (!m_startup->setEnabled(false))
            return Outcome::Rejected;
        return Outcome::Plain;
    }

    CaptureLocations::Kind kind;
    if (captureRootKind(key, kind)) {
        QString error;
        // A failed reset stays silent, matching the previous resetConfig path:
        // the settings page re-reads the root and shows whatever is still set.
        if (!m_locations->resetBaseRoot(kind, &error))
            return Outcome::Consumed;
        return Outcome::Rescan;
    }

    return Outcome::Plain;
}
