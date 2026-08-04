#pragma once

#include <QSet>
#include <QString>
#include <QStringList>

namespace ModernInput {

class SelectiveRawHidFallback
{
public:
    static SelectiveRawHidFallback& instance();

    void beginProbe();
    void endProbe();
    void setBoundControls(const QStringList& controlIds);
    // Bumped on every setBoundControls/probe transition; the Raw Input
    // backend keys its per-handle eligibility cache on this so a newly bound
    // control starts producing without a replug.
    int generation() const { return m_generation; }
    // Idle fast-path gate: with no bound raw-HID control and no open probe,
    // the Raw Input backend skips ALL fallback bookkeeping (one branch, zero
    // allocation — the flood guarantee).
    bool hasAnyBindings() const { return !m_boundControls.isEmpty(); }
    // True when any bound raw-HID control belongs to this device identity —
    // the cheap gate that decides whether a device's reports are worth a
    // payload read at all on the production path.
    bool hasBindingsFor(const QString& deviceIdentity) const;
    bool shouldObserve(const QString& deviceIdentity, quint16 usagePage, quint16 usage) const;
    QString observeUsage(const QString& deviceIdentity, quint16 usagePage,
                         quint16 usage, bool pressed);
    void observeKeyboard(const QString& keyName);
    QString probeSummary() const;
    bool probeActive() const { return m_probeActive; }

private:
    bool m_probeActive = false;
    int m_generation = 0;
    QSet<QString> m_boundControls;
    QStringList m_observations;
};

} // namespace ModernInput
