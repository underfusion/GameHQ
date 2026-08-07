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
    ++m_generation;
    m_observations.clear();
}

void SelectiveRawHidFallback::endProbe()
{
    m_probeActive = false;
    ++m_generation;
}

void SelectiveRawHidFallback::setBoundControls(const QStringList& controlIds)
{
    m_boundControls = QSet<QString>(controlIds.cbegin(), controlIds.cend());
    ++m_generation;
}

bool SelectiveRawHidFallback::hasBindingsFor(const QString& deviceIdentity) const
{
    if (m_boundControls.isEmpty())
        return false;
    // rawHidUsage() anonymizes the identity into the code's third segment;
    // rebuild that prefix and match. Called only on eligibility-cache misses.
    const QString prefix = ControlId::rawHidUsage(deviceIdentity, 0, 0)
        .section(QLatin1Char('.'), 0, 2) + QLatin1Char('.');
    for (const QString& control : m_boundControls) {
        if (control.startsWith(prefix))
            return true;
    }
    return false;
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
                              "The controller's current firmware mode may disable this button "
                              "entirely — try another controller mode or close software that "
                              "intercepts buttons.");
    }
    // A button whose only trace is a keyboard event is most likely a firmware
    // macro key (GameSir-style Share buttons emit the Windows screenshot
    // shortcut, not a gamepad control) — but the probe cannot prove the key
    // came from the pad, so the copy says "may", not "is".
    bool keyboardOnly = true;
    for (const QString& observation : m_observations) {
        if (!observation.startsWith(QLatin1String("Keyboard macro:"))) {
            keyboardOnly = false;
            break;
        }
    }
    if (keyboardOnly) {
        return m_observations.join(QStringLiteral("; "))
            + QStringLiteral(". Only keyboard input was observed during this probe — the "
                             "button may be configured as a keyboard macro. Assign it in the "
                             "Keyboard section, or remap it in the vendor's software to an "
                             "unused key such as F13 first.");
    }
    return m_observations.join(QStringLiteral("; "));
}

} // namespace ModernInput
