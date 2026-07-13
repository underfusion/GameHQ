#include "input/BindingEditorModel.h"

#include "input/ActionCatalog.h"
#include "input/BindingRuntime.h"
#include "storage/CaptureDatabase.h"

#include <QHash>
#include <QVariantMap>
#include <utility>

namespace {
QString slotKey(const QString& actionId, int slot)
{
    return actionId + QLatin1Char('#') + QString::number(slot);
}
}

BindingEditorModel::BindingEditorModel(CaptureDatabase* database, BindingRuntime* runtime,
                                       std::function<void()> reloadRuntime, QObject* parent)
    : QObject(parent)
    , m_database(database)
    , m_runtime(runtime)
    , m_reloadRuntime(std::move(reloadRuntime))
{
    rebuildRows();
}

void BindingEditorModel::setDeviceGroup(const QString& group)
{
    if (group != QLatin1String("controller") && group != QLatin1String("keyboard")
        && group != QLatin1String("mouse"))
        return;
    if (m_deviceGroup == group)
        return;
    cancelCapture();
    m_deviceGroup = group;
    emit deviceGroupChanged();
    rebuildRows();
}

void BindingEditorModel::setControllerSpecific(bool specific)
{
    specific = specific && controllerSpecificAvailable();
    if (m_controllerSpecific == specific)
        return;
    cancelCapture();
    m_controllerSpecific = specific;
    emit controllerSpecificChanged();
    rebuildRows();
}

QString BindingEditorModel::selectedProfile() const
{
    return m_deviceGroup == QLatin1String("controller") && m_controllerSpecific
        ? m_controllerFingerprint : QString();
}

QString BindingEditorModel::scopeLabel(ActionCatalog::Scope scope)
{
    switch (scope) {
    case ActionCatalog::Scope::Global: return QStringLiteral("Global");
    case ActionCatalog::Scope::Overlay: return QStringLiteral("Overlay");
    case ActionCatalog::Scope::Desktop: return QStringLiteral("Gallery");
    case ActionCatalog::Scope::Playback: return QStringLiteral("Playback");
    }
    return {};
}

QString BindingEditorModel::formatBinding(const BindingResolver::Binding& binding) const
{
    if (binding.triggerCode.isEmpty())
        return QStringLiteral("Unassigned");
    QString label = binding.triggerCode;
    if (m_deviceGroup == QLatin1String("controller"))
        label = ControlId::label(binding.triggerCode, m_controllerFamily);
    else if (m_deviceGroup == QLatin1String("mouse")) {
        if (binding.triggerCode == QLatin1String("mouse.button4")) label = QStringLiteral("Mouse Back");
        else if (binding.triggerCode == QLatin1String("mouse.button5")) label = QStringLiteral("Mouse Forward");
        else if (binding.triggerCode == QLatin1String("mouse.middle")) label = QStringLiteral("Middle Mouse");
    }
    if (binding.activation == QLatin1String("tap"))
        label += QStringLiteral(" · Tap");
    else if (binding.activation == QLatin1String("hold"))
        label += QStringLiteral(" · Hold");
    else if (binding.activation == QLatin1String("double_tap"))
        label += QStringLiteral(" · Double tap");
    return label;
}

void BindingEditorModel::rebuildRows()
{
    QHash<QString, BindingResolver::Binding> bindings;
    for (const auto& binding : m_runtime->effectiveBindings(m_deviceGroup, selectedProfile()))
        bindings.insert(slotKey(binding.actionId, binding.slot), binding);

    QVariantList next;
    for (const auto& action : ActionCatalog::all()) {
        QVariantMap row;
        row.insert(QStringLiteral("actionId"), action.id);
        row.insert(QStringLiteral("label"), action.label);
        row.insert(QStringLiteral("description"), action.description);
        row.insert(QStringLiteral("scope"), scopeLabel(action.scope));
        row.insert(QStringLiteral("bindable"), action.bindable);
        for (int slot = 1; slot <= 2; ++slot) {
            const auto binding = bindings.value(slotKey(action.id, slot));
            row.insert(slot == 1 ? QStringLiteral("primary") : QStringLiteral("secondary"),
                       binding.actionId.isEmpty() ? QStringLiteral("Unassigned") : formatBinding(binding));
        }
        next.append(row);
    }
    m_rows = next;
    emit rowsChanged();
}

void BindingEditorModel::beginCapture(const QString& actionId, int slot)
{
    const auto* action = ActionCatalog::find(actionId);
    if (!action || !action->bindable || slot < 1 || slot > 2)
        return;
    m_captureActionId = actionId;
    m_captureSlot = slot;
    m_captureActive = true;
    const QString device = m_deviceGroup == QLatin1String("controller") ? QStringLiteral("controller button")
                         : m_deviceGroup == QLatin1String("keyboard") ? QStringLiteral("key or shortcut")
                                                                      : QStringLiteral("middle, Back, or Forward mouse button");
    m_capturePrompt = QStringLiteral("Press a %1 for %2 · Slot %3")
                          .arg(device, action->label).arg(slot);
    emit captureChanged();
}

void BindingEditorModel::cancelCapture()
{
    if (!m_captureActive)
        return;
    m_captureActive = false;
    m_captureActionId.clear();
    m_capturePrompt.clear();
    emit captureChanged();
}

bool BindingEditorModel::scopesConflict(ActionCatalog::Scope left, ActionCatalog::Scope right)
{
    return left == right;
}

bool BindingEditorModel::captureInput(const QString& deviceGroup, const QString& triggerCode,
                                      const QString& displayLabel)
{
    if (!m_captureActive || deviceGroup != m_deviceGroup || triggerCode.isEmpty())
        return false;

    const QString profile = selectedProfile();
    const auto effective = m_runtime->effectiveBindings(deviceGroup, profile);
    BindingResolver::Binding target{deviceGroup, profile, m_captureActionId, m_captureSlot,
                                    triggerCode, QStringLiteral("press"), 0, false};
    for (const auto& binding : effective) {
        if (binding.actionId == m_captureActionId && binding.slot == m_captureSlot) {
            target.activation = binding.activation;
            target.holdMs = binding.holdMs;
            break;
        }
    }
    if (deviceGroup != QLatin1String("controller")) {
        target.activation = QStringLiteral("press");
        target.holdMs = 0;
    }

    PendingChange change;
    change.target = target;
    const auto* targetAction = ActionCatalog::find(target.actionId);
    for (const auto& binding : effective) {
        if (binding.actionId == target.actionId && binding.slot == target.slot)
            continue;
        if (binding.triggerCode != target.triggerCode || binding.activation != target.activation)
            continue;
        const auto* otherAction = ActionCatalog::find(binding.actionId);
        if (targetAction && otherAction && scopesConflict(targetAction->scope, otherAction->scope))
            change.conflicts.append(binding);
    }

    m_captureActive = false;
    m_capturePrompt.clear();
    emit captureChanged();
    if (!change.conflicts.isEmpty()) {
        m_pending = change;
        m_conflictPending = true;
        const auto* other = ActionCatalog::find(change.conflicts.first().actionId);
        m_conflictMessage = QStringLiteral("%1 is already assigned to %2 in %3. Replace that assignment?")
                                .arg(displayLabel,
                                     other ? other->label : change.conflicts.first().actionId,
                                     other ? scopeLabel(other->scope) : QStringLiteral("this context"));
        emit conflictChanged();
        return true;
    }
    applyChange(change);
    return true;
}

void BindingEditorModel::applyChange(const PendingChange& change)
{
    for (const auto& conflict : change.conflicts) {
        BindingOverrideRow row{conflict.deviceGroup, selectedProfile(), conflict.actionId,
                               conflict.slot, {}, conflict.activation, conflict.holdMs, true};
        m_database->upsertBindingOverride(row);
    }
    const auto& target = change.target;
    BindingOverrideRow row{target.deviceGroup, selectedProfile(), target.actionId, target.slot,
                           target.triggerCode, target.activation, target.holdMs, false};
    m_database->upsertBindingOverride(row);
    reloadAndRefresh();
}

void BindingEditorModel::clearBinding(const QString& actionId, int slot)
{
    const auto* action = ActionCatalog::find(actionId);
    if (!action || !action->bindable)
        return;
    BindingOverrideRow row{m_deviceGroup, selectedProfile(), actionId, slot, {},
                           QStringLiteral("press"), 0, true};
    m_database->upsertBindingOverride(row);
    reloadAndRefresh();
}

void BindingEditorModel::resetAction(const QString& actionId)
{
    for (int slot = 1; slot <= 2; ++slot)
        m_database->clearBindingOverride(m_deviceGroup, selectedProfile(), actionId, slot);
    reloadAndRefresh();
}

void BindingEditorModel::resetCurrentProfile()
{
    m_database->clearBindingOverridesForProfile(m_deviceGroup, selectedProfile());
    reloadAndRefresh();
}

void BindingEditorModel::resetAllBindings()
{
    m_database->clearAllBindingOverrides();
    reloadAndRefresh();
}

void BindingEditorModel::confirmConflict()
{
    if (!m_conflictPending)
        return;
    const PendingChange change = m_pending;
    dismissConflict();
    applyChange(change);
}

void BindingEditorModel::dismissConflict()
{
    if (!m_conflictPending)
        return;
    m_conflictPending = false;
    m_conflictMessage.clear();
    m_pending = {};
    emit conflictChanged();
}

void BindingEditorModel::reloadAndRefresh()
{
    if (m_reloadRuntime)
        m_reloadRuntime();
    rebuildRows();
}

void BindingEditorModel::setControllerProfile(const ControlId::DeviceProfile& profile)
{
    const bool availabilityChanged = m_controllerFingerprint.isEmpty() != profile.fingerprint.isEmpty();
    m_controllerFingerprint = profile.fingerprint;
    m_controllerName = profile.displayName;
    m_controllerFamily = profile.family;
    if (m_controllerSpecific && m_controllerFingerprint.isEmpty()) {
        m_controllerSpecific = false;
        emit controllerSpecificChanged();
    }
    emit controllerProfileChanged();
    if (m_deviceGroup == QLatin1String("controller") || availabilityChanged)
        rebuildRows();
}

void BindingEditorModel::setLastFiredAction(const QString& actionId)
{
    const auto* action = ActionCatalog::find(actionId);
    const QString text = action ? action->label : actionId;
    if (m_lastFiredAction == text)
        return;
    m_lastFiredAction = text;
    emit lastFiredActionChanged();
}
