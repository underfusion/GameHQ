#include "input/InputDiagnostics.h"

#include <QRegularExpression>

InputDiagnostics& InputDiagnostics::instance()
{
    static InputDiagnostics diagnostics;
    return diagnostics;
}

InputDiagnostics::InputDiagnostics()
{
    m_clock.start();
}

void InputDiagnostics::setPreviousSessionCrashed(bool crashed)
{
    m_previousSessionCrashed = crashed;
}

void InputDiagnostics::noteBackendSwitch(const QString& backend, const QString& reason)
{
    m_activeBackend = backend;
    push(m_switches, kMaxSwitches, m_clock.elapsed(),
         QStringLiteral("%1 (%2)").arg(backend, reason));
}

void InputDiagnostics::noteDevice(const QString& identity, const QString& description,
                                  const QString& verdict)
{
    if (!m_devices.contains(identity))
        m_deviceOrder.append(identity);
    DeviceInfo& info = m_devices[identity];
    info.description = description;
    info.verdict = verdict;
}

void InputDiagnostics::noteRate(const QString& identity, quint32 eventsPerSecond)
{
    if (!m_devices.contains(identity))
        m_deviceOrder.append(identity);
    DeviceInfo& info = m_devices[identity];
    info.eventsPerSecond = eventsPerSecond;
    info.sawRate = true;
}

void InputDiagnostics::noteControl(const QString& controlId, const QString& backend)
{
    push(m_controls, kMaxControls, m_clock.elapsed(),
         QStringLiteral("%1 (%2)").arg(controlId, backend));
}

void InputDiagnostics::noteForeground(const QString& phase, bool acquired)
{
    push(m_foreground, kMaxForeground, m_clock.elapsed(),
         QStringLiteral("%1 %2").arg(phase,
                                     acquired ? QStringLiteral("acquired")
                                              : QStringLiteral("FAILED")));
}

void InputDiagnostics::setCloakStatus(const QStringList& hiddenPads, bool hidHidePresent)
{
    m_hiddenPads = hiddenPads;
    m_hidHidePresent = hidHidePresent;
}

void InputDiagnostics::startProbe(int durationMs)
{
    m_probeStartedMs = m_clock.elapsed();
    m_probeDeadlineMs = m_probeStartedMs + qBound(250, durationMs, 10000);
    m_probeEvents.clear();
    m_probeOverflowed = false;
    m_probeSampled = false;
}

bool InputDiagnostics::probeActive() const
{
    return m_probeDeadlineMs >= 0 && m_clock.elapsed() < m_probeDeadlineMs;
}

bool InputDiagnostics::noteProbeEvent(const QString& identity, const QString& backend,
                                      const QString& detail)
{
    if (!probeActive())
        return false;
    if (m_probeEvents.size() >= kMaxProbeEvents) {
        m_probeOverflowed = true;
        return false;
    }
    push(m_probeEvents, kMaxProbeEvents, m_clock.elapsed() - m_probeStartedMs,
         QStringLiteral("%1 via %2: %3").arg(identity, backend, detail));
    return true;
}

void InputDiagnostics::noteProbeSampled()
{
    if (probeActive())
        m_probeSampled = true;
}

QString InputDiagnostics::probeSummary() const
{
    if (m_probeStartedMs < 0)
        return QStringLiteral("Probe: never run");
    QStringList lines;
    lines << (probeActive() ? QStringLiteral("Probe: RUNNING")
                            : QStringLiteral("Probe: finished"));
    if (m_probeEvents.isEmpty()) {
        lines << QStringLiteral("  no button change reached GameHQ during the window");
    } else {
        for (const Stamped& event : m_probeEvents)
            lines << QStringLiteral("  +%1ms %2").arg(event.ms).arg(event.text);
    }
    if (m_probeOverflowed)
        lines << QStringLiteral("  (event cap reached, further changes dropped)");
    if (m_probeSampled)
        lines << QStringLiteral("  (high report rate: reports were sampled, not read "
                                "one by one — press and hold the button if nothing "
                                "showed up)");
    return lines.join(QLatin1Char('\n'));
}

QString InputDiagnostics::redactDevicePath(const QString& path)
{
    if (path.isEmpty())
        return path;
    // Keep only what a bug report needs to identify the model. Serial numbers,
    // container ids and instance paths all collapse into the hash.
    static const QRegularExpression vidPid(
        QStringLiteral("vid_([0-9a-fA-F]{4}).*?pid_([0-9a-fA-F]{4})"),
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match = vidPid.match(path);
    const QString identity = match.hasMatch()
        ? QStringLiteral("%1:%2").arg(match.captured(1).toUpper(),
                                      match.captured(2).toUpper())
        : QStringLiteral("????:????");
    return QStringLiteral("%1/#%2").arg(
        identity, QString::number(qHash(path), 16).right(8));
}

QString InputDiagnostics::exportText() const
{
    QStringList lines;
    lines << QStringLiteral("Input:");
    lines << QStringLiteral("  Previous session ended unexpectedly: %1")
                 .arg(m_previousSessionCrashed ? QStringLiteral("YES")
                                               : QStringLiteral("no"));
    lines << QStringLiteral("  Active backend: %1")
                 .arg(m_activeBackend.isEmpty() ? QStringLiteral("none")
                                                : m_activeBackend);

    lines << QStringLiteral("  Backend switches:");
    if (m_switches.isEmpty())
        lines << QStringLiteral("    none");
    for (const Stamped& entry : m_switches)
        lines << QStringLiteral("    %1").arg(stamp(entry));

    lines << QStringLiteral("  Devices:");
    if (m_deviceOrder.isEmpty())
        lines << QStringLiteral("    none seen");
    for (const QString& identity : m_deviceOrder) {
        const DeviceInfo& info = m_devices.value(identity);
        QString line = QStringLiteral("    %1 %2 -> %3")
                           .arg(identity, info.description, info.verdict);
        if (info.sawRate)
            line += QStringLiteral(" @ %1 events/s").arg(info.eventsPerSecond);
        lines << line;
    }

    lines << QStringLiteral("  Recent controls:");
    if (m_controls.isEmpty())
        lines << QStringLiteral("    none");
    for (const Stamped& entry : m_controls)
        lines << QStringLiteral("    %1").arg(stamp(entry));

    lines << QStringLiteral("  Overlay foreground:");
    if (m_foreground.isEmpty())
        lines << QStringLiteral("    no overlay open/close this session");
    for (const Stamped& entry : m_foreground)
        lines << QStringLiteral("    %1").arg(stamp(entry));

    if (m_hiddenPads.isEmpty()) {
        lines << QStringLiteral("  Controllers hidden by a HID filter: none");
    } else {
        lines << QStringLiteral("  Controllers hidden by a HID filter (HidHide %1):")
                     .arg(m_hidHidePresent ? QStringLiteral("installed")
                                           : QStringLiteral("not detected"));
        for (const QString& pad : m_hiddenPads)
            lines << QStringLiteral("    %1").arg(pad);
    }

    lines << QStringLiteral("  %1").arg(probeSummary());
    return lines.join(QLatin1Char('\n'));
}

void InputDiagnostics::clear()
{
    m_previousSessionCrashed = false;
    m_switches.clear();
    m_controls.clear();
    m_foreground.clear();
    m_devices.clear();
    m_deviceOrder.clear();
    m_hiddenPads.clear();
    m_hidHidePresent = false;
    m_activeBackend.clear();
    m_probeDeadlineMs = -1;
    m_probeStartedMs = -1;
    m_probeEvents.clear();
    m_probeOverflowed = false;
    m_probeSampled = false;
}

void InputDiagnostics::push(QVector<Stamped>& ring, int cap, qint64 ms,
                            const QString& text)
{
    if (ring.size() >= cap)
        ring.removeFirst();
    ring.append({ms, text});
}

QString InputDiagnostics::stamp(const Stamped& entry)
{
    return QStringLiteral("+%1s %2")
        .arg(QString::number(entry.ms / 1000.0, 'f', 1), entry.text);
}
