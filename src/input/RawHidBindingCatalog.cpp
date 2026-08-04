#include "input/RawHidBindingCatalog.h"

#include "input/BindingPattern.h"
#include "input/BindingRuntime.h"
#include "input/ControlId.h"
#include "storage/CaptureDatabase.h"

#include <QSet>

#include <algorithm>

namespace ModernInput {

QStringList persistedRawHidControls(const BindingRuntime& runtime,
                                    const CaptureDatabase* database)
{
    QSet<QString> controls;
    const auto addTrigger = [&](const QString& serialized) {
        const auto parsed = TriggerSpec::parse(serialized);
        if (!parsed.ok)
            return;
        for (const QString& control : parsed.trigger.controls) {
            if (ControlId::isRawHidUsage(control))
                controls.insert(control);
        }
    };

    for (const auto& binding : runtime.effectiveBindings(QStringLiteral("controller"))) {
        if (!binding.unbound)
            addTrigger(binding.triggerCode);
    }
    if (database) {
        for (const BindingOverrideRow& row : database->listBindingOverrides()) {
            if (row.deviceGroup == QLatin1String("controller") && !row.unbound)
                addTrigger(row.triggerCode);
        }
    }

    QStringList result(controls.cbegin(), controls.cend());
    std::sort(result.begin(), result.end());
    return result;
}

} // namespace ModernInput
