#pragma once

#include <QString>
#include <QVariant>

class CaptureLocations;
class StartupManager;

// Most config keys are plain values that ConfigManager stores and returns. A few
// are not: they drive a side effect that can refuse the change (the Windows
// startup entry), or they are owned by a delegate that persists them itself (the
// capture roots, via CaptureLocations). SettingsRouter holds that special-casing
// so AppController::setConfig/resetConfig stay a plain read of the outcome.
//
// The router deliberately performs the side effect but emits no signals and does
// no rescan: those belong to AppController, which acts on the returned Outcome.
class SettingsRouter
{
public:
    enum class Outcome
    {
        Plain,      // not special, or the side effect succeeded — apply the normal config write
        Rejected,   // the side effect refused the change — re-emit the stored value
        Consumed,   // a delegate handled it, including its own persistence
        Rescan,     // consumed, and the capture library must be rescanned
    };

    SettingsRouter(StartupManager* startup, CaptureLocations* locations);

    Outcome change(const QString& key, const QVariant& value);
    Outcome reset(const QString& key);

private:
    StartupManager* m_startup;
    CaptureLocations* m_locations;
};
