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
    bool shouldObserve(const QString& deviceIdentity, quint16 usagePage, quint16 usage) const;
    QString observeUsage(const QString& deviceIdentity, quint16 usagePage,
                         quint16 usage, bool pressed);
    void observeKeyboard(const QString& keyName);
    QString probeSummary() const;
    bool probeActive() const { return m_probeActive; }

private:
    bool m_probeActive = false;
    QSet<QString> m_boundControls;
    QStringList m_observations;
};

} // namespace ModernInput
