#pragma once

#include <QDateTime>

// When an automatic update check is allowed to run.
//
// Two independent limits apply. The daily interval is GameHQ's own politeness
// budget. The cooldown is GitHub's: after it reports a rate limit, checking
// again before the reset it named is guaranteed to fail and spends quota that
// is already exhausted. The cooldown used to be reported to the user and then
// forgotten, so the hourly wake walked straight back into the limit.
namespace UpdateSchedule
{
// GitHub's anonymous budget refills hourly, so this is the longest a rate limit
// can sensibly last when it declines to say when it resets.
inline constexpr qint64 kDefaultCooldownSecs = 15 * 60;
inline constexpr qint64 kAutomaticIntervalSecs = 24 * 60 * 60;

// All times UTC. An invalid lastChecked means "never checked"; an invalid
// nextAllowed means "no cooldown".
bool automaticCheckAllowed(const QDateTime &lastChecked, const QDateTime &nextAllowed,
                           const QDateTime &now,
                           qint64 intervalSecs = kAutomaticIntervalSecs);

// True while a rate-limit cooldown is still in force, which is also what makes
// a manual check report the retry time instead of firing a doomed request.
bool inCooldown(const QDateTime &nextAllowed, const QDateTime &now);

// When a rate limit that resets at `resetAt` allows the next attempt. An
// invalid or already-past reset falls back to a fixed cooldown rather than
// permitting an immediate retry.
QDateTime cooldownUntil(const QDateTime &resetAt, const QDateTime &now,
                        qint64 defaultCooldownSecs = kDefaultCooldownSecs);
}
