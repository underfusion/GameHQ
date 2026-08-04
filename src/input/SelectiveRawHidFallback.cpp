#include "input/SelectiveRawHidFallback.h"

#include "input/ControlId.h"

namespace ModernInput {

SelectiveRawHidFallback& SelectiveRawHidFallback::instance()
{
    static SelectiveRawHidFallback value;
    return value;
}

void SelectiveRawHidFallback::beginProbe()
{
    m_probeActive = true;
    m_observations.clear();
}

void SelectiveRawHidFallback::endProbe()
{
    m_probeActive = false;
}

void SelectiveRawHidFallback::setBoundControls(const QStringList& controlIds)
{
    m_boundControls = QSet<QString>(controlIds.cbegin(), controlIds.cend());
}

bool SelectiveRawHidFallback::shouldObserve(const QString& deviceIdentity,
                                            quint16 usagePage, quint16 usage) const
{
    return m_probeActive
        || m_boundControls.contains(ControlId::rawHidUsage(deviceIdentity, usagePage, usage));
}

QString SelectiveRawHidFallback::observeUsage(const QString& deviceIdentity,
                                              quint16 usagePage, quint16 usage,
                                              bool pressed)
{
    const QString control = ControlId::rawHidUsage(deviceIdentity, usagePage, usage);
    if (!m_probeActive && !m_boundControls.contains(control))
        return {};
    if (m_probeActive) {
        m_observations.push_back(QStringLiteral("Raw HID %1 %2")
            .arg(ControlId::label(control, ControlId::ControllerFamily::Generic),
                 pressed ? QStringLiteral("pressed") : QStringLiteral("released")));
    }
    return control;
}

void SelectiveRawHidFallback::observeKeyboard(const QString& keyName)
{
    if (!m_probeActive || keyName.isEmpty())
        return;
    m_observations.push_back(QStringLiteral("Keyboard macro: %1").arg(keyName));
}

QString SelectiveRawHidFallback::probeSummary() const
{
    if (m_observations.isEmpty()) {
        return QStringLiteral("No event was received through GameInput, Raw HID, or keyboard. "
                              "Try another controller mode or close software that intercepts buttons.");
    }
    return m_observations.join(QStringLiteral("; "));
}

} // namespace ModernInput
