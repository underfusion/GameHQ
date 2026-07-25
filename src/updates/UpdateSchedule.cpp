#include "updates/UpdateSchedule.h"

namespace UpdateSchedule
{
bool inCooldown(const QDateTime &nextAllowed, const QDateTime &now)
{
    return nextAllowed.isValid() && now.isValid() && now < nextAllowed;
}

bool automaticCheckAllowed(const QDateTime &lastChecked, const QDateTime &nextAllowed,
                           const QDateTime &now, qint64 intervalSecs)
{
    if (inCooldown(nextAllowed, now))
        return false;
    if (!lastChecked.isValid())
        return true;   // never checked
    // A clock that moved backwards must not lock checking out until it catches
    // up: treat a future timestamp as "due".
    if (now < lastChecked)
        return true;
    return lastChecked.secsTo(now) >= intervalSecs;
}

QDateTime cooldownUntil(const QDateTime &resetAt, const QDateTime &now, qint64 defaultCooldownSecs)
{
    const QDateTime fallback = now.addSecs(defaultCooldownSecs);
    if (!resetAt.isValid() || resetAt <= now)
        return fallback;
    return resetAt;
}
}
