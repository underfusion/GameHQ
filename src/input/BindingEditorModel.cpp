#include "input/BindingEditorModel.h"

#include "input/ActionCatalog.h"
#include "input/BindingRelation.h"
#include "input/BindingRuntime.h"
#include "input/ControllerIdentity.h"
#include "storage/CaptureDatabase.h"

#include <QHash>
#include <QSet>
#include <QVariantMap>
#include <utility>

namespace {
QString slotKey(const QString& actionId, int slot)
{
    return actionId + QLatin1Char('#') + QString::number(slot);
}

// A new binding can relate to several existing ones at once. effectiveBindings()
// comes out of a QHash, so "first match wins" would show a different notice on
// different runs. Rank the non-blocking tiers by how much the user needs to
// know: a context override changes what the button does, a redundant binding
// means their edit did nothing, and a shared gesture is merely informational.
int noticeSeverity(BindingRelation::Kind kind)
{
    switch (kind) {
    case BindingRelation::Kind::ContextOverride: return 3;
    case BindingRelation::Kind::Redundant:       return 2;
    case BindingRelation::Kind::SharedGesture:   return 1;
    case BindingRelation::Kind::HardConflict:            // owns the modal instead
    case BindingRelation::Kind::None:            return 0;
    }
    return 0;
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
    // Controllers assign a gesture, not just a button — say which one, so
    // "capture Screenshot" reads as the Tap it will actually become. Keyboard
    // and mouse bindings are always plain presses; no suffix there.
    if (m_deviceGroup == QLatin1String("controller")) {
        const BindingResolver::Gesture gesture =
            m_runtime->inheritedGesture(m_deviceGroup, selectedProfile(), actionId, slot);
        m_capturePrompt += QStringLiteral(" · %1").arg(gestureLabel(gesture.activation));
    }
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

void BindingEditorModel::setRelationNotice(const QString& kindId, const QString& text)
{
    // Three distinct failures, three kinds — they are never the same problem:
    // unsupported_input  = the controller/backend cannot expose this control;
    // hotkey_unavailable = Windows or another app owns the chord;
    // persistence_error  = the write itself failed.
    // All three populate validationError so QML can style them as errors
    // without parsing the message.
    const bool blocking = kindId == QLatin1String("unsupported_input")
                       || kindId == QLatin1String("hotkey_unavailable")
                       || kindId == QLatin1String("persistence_error");
    const QString error = blocking ? text : QString();
    if (m_relationKind == kindId && m_relationNotice == text && m_validationError == error)
        return;
    m_relationKind = kindId;
    m_relationNotice = text;
    m_validationError = error;
    emit relationNoticeChanged();
}

void BindingEditorModel::reportUnsupportedInput(const QString& displayLabel)
{
    setRelationNotice(QStringLiteral("unsupported_input"),
                      displayLabel.isEmpty()
                          ? QStringLiteral("This controller button is not exposed by the active input backend.")
                          : QStringLiteral("%1 is not exposed by the active input backend.")
                                .arg(displayLabel));
}

void BindingEditorModel::dismissRelationNotice()
{
    setRelationNotice(QStringLiteral("none"), {});
}

bool BindingEditorModel::captureInput(const QString& deviceGroup, const QString& triggerCode,
                                      const QString& displayLabel)
{
    if (!m_captureActive || deviceGroup != m_deviceGroup || triggerCode.isEmpty())
        return false;

    const QString profile = selectedProfile();
    const auto effective = m_runtime->effectiveBindings(deviceGroup, profile);
    // The gesture belongs to the slot, not to the trigger being captured: an
    // empty secondary Screenshot is still a tap, a cleared Save Replay is
    // still a hold. Inheriting it here keeps the relation classification below
    // honest — a guessed "press" manufactured HardConflicts on any trigger
    // that already carried a timed gesture.
    const BindingResolver::Gesture gesture =
        m_runtime->inheritedGesture(deviceGroup, profile, m_captureActionId, m_captureSlot);
    BindingResolver::Binding target{deviceGroup, profile, m_captureActionId, m_captureSlot,
                                    triggerCode, gesture.activation, gesture.holdMs, false};
    if (deviceGroup != QLatin1String("controller")) {
        target.activation = QStringLiteral("press");
        target.holdMs = 0;
    }

    // One shared policy decides what the new binding means next to every
    // existing one. Only HardConflict blocks; the softer tiers are recorded so
    // the editor can explain the result without refusing a valid assignment.
    PendingChange change;
    change.target = target;
    BindingRelation::Kind notice = BindingRelation::Kind::None;
    BindingResolver::Binding noticePartner;
    for (const auto& binding : effective) {
        if (binding.actionId == target.actionId && binding.slot == target.slot)
            continue;
        const BindingRelation::Kind kind = BindingRelation::classify(target, binding);
        if (kind == BindingRelation::Kind::HardConflict) {
            change.conflicts.append(binding);
        } else if (noticeSeverity(kind) > noticeSeverity(notice)) {
            notice = kind;
            noticePartner = binding;
        }
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
        setRelationNotice(BindingRelation::kindId(BindingRelation::Kind::HardConflict),
                          m_conflictMessage);
        emit conflictChanged();
        return true;
    }

    // A rejected chord or a failed write leaves its own explanation in place;
    // do not overwrite it with a success notice.
    if (!applyChange(change))
        return true;
    setRelationNotice(BindingRelation::kindId(notice),
                      noticeTextFor(notice, target, noticePartner, displayLabel));
    return true;
}

QString BindingEditorModel::noticeTextFor(BindingRelation::Kind kind,
                                          const BindingResolver::Binding& target,
                                          const BindingResolver::Binding& partner,
                                          const QString& displayLabel) const
{
    if (kind == BindingRelation::Kind::None)
        return {};
    const auto* targetAction = ActionCatalog::find(target.actionId);
    const auto* partnerAction = ActionCatalog::find(partner.actionId);
    if (!targetAction || !partnerAction)
        return {};

    switch (kind) {
    case BindingRelation::Kind::ContextOverride:
        // Saved, but the user should know one of the two only runs in context.
        return QStringLiteral("%1 replaces %2 while %3 is active. Both are saved.")
            .arg(targetAction->label, partnerAction->label,
                 scopeLabel(targetAction->scope == ActionCatalog::Scope::Global
                                ? partnerAction->scope : targetAction->scope));
    case BindingRelation::Kind::Redundant:
        return QStringLiteral("%1 already does the same thing here. The duplicate has no effect.")
            .arg(partnerAction->label);
    case BindingRelation::Kind::SharedGesture:
        return QStringLiteral("%1 is shared: %2 = %3, %4 = %5.")
            .arg(displayLabel,
                 gestureLabel(target.activation), targetAction->label,
                 gestureLabel(partner.activation), partnerAction->label);
    case BindingRelation::Kind::HardConflict:
    case BindingRelation::Kind::None:
        break;
    }
    return {};
}

QString BindingEditorModel::gestureLabel(const QString& activation)
{
    if (activation == QLatin1String("tap")) return QStringLiteral("Tap");
    if (activation == QLatin1String("hold")) return QStringLiteral("Hold");
    if (activation == QLatin1String("double_tap")) return QStringLiteral("Double tap");
    return QStringLiteral("Press");
}

bool BindingEditorModel::legacyCopyAvailable() const
{
    if (m_deviceGroup != QLatin1String("controller") || !m_controllerSpecific)
        return false;
    const QString profile = selectedProfile();
    if (profile.isEmpty() || ControllerIdentity::isLegacySlotFingerprint(profile))
        return false;
    for (const BindingOverrideRow& row : m_database->listBindingOverrides()) {
        if (row.deviceGroup == QLatin1String("controller")
            && ControllerIdentity::isLegacySlotFingerprint(row.deviceProfile))
            return true;
    }
    return false;
}

void BindingEditorModel::copyLegacyOverridesToController()
{
    if (!legacyCopyAvailable())
        return;
    const QString profile = selectedProfile();
    const auto rows = m_database->listBindingOverrides();
    QSet<QString> owned;   // keys this controller already has its own row for
    for (const BindingOverrideRow& row : rows) {
        if (row.deviceGroup == QLatin1String("controller") && row.deviceProfile == profile)
            owned.insert(row.actionId + QLatin1Char('#') + QString::number(row.slot));
    }
    for (BindingOverrideRow row : rows) {
        if (row.deviceGroup != QLatin1String("controller")
            || !ControllerIdentity::isLegacySlotFingerprint(row.deviceProfile))
            continue;
        // Copy, never move — the slot rows keep covering pads that cannot be
        // identified — and never clobber a row already saved for this pad.
        if (owned.contains(row.actionId + QLatin1Char('#') + QString::number(row.slot)))
            continue;
        row.deviceProfile = profile;
        m_database->upsertBindingOverride(row);
    }
    reloadAndRefresh();
}

void BindingEditorModel::retryConflictCapture()
{
    if (!m_conflictPending)
        return;
    const QString actionId = m_pending.target.actionId;
    const int slot = m_pending.target.slot;
    dismissConflict();
    beginCapture(actionId, slot);
}

void BindingEditorModel::setHotkeyApply(HotkeyApply apply)
{
    m_hotkeyApply = std::move(apply);
}

void BindingEditorModel::setPersistRow(PersistRow persist)
{
    m_persistRow = std::move(persist);
}

bool BindingEditorModel::persist(const BindingOverrideRow& row)
{
    return m_persistRow ? m_persistRow(row) : m_database->upsertBindingOverride(row);
}

bool BindingEditorModel::isGlobalHotkey(const BindingResolver::Binding& binding) const
{
    if (binding.deviceGroup != QLatin1String("keyboard"))
        return false;
    if (binding.activation != QLatin1String("press"))
        return false;
    const auto* action = ActionCatalog::find(binding.actionId);
    return action && action->scope == ActionCatalog::Scope::Global;
}

// Commits the OS registration and the database write together, or leaves both
// exactly as they were. Previously the DB was written first and the hotkey layer
// was reconciled afterwards from InputEngine, whose return value was discarded:
// a chord Windows refused was still saved, so Settings showed a shortcut that
// did nothing until the next restart.
bool BindingEditorModel::applyChange(const PendingChange& change)
{
    const auto& target = change.target;
    const QString profile = selectedProfile();
    const bool ownsHotkey = m_hotkeyApply && isGlobalHotkey(target);

    // 1) Remember what is live now, so step 4 can put it back.
    QString previousChord;
    if (ownsHotkey) {
        for (const auto& binding : m_runtime->effectiveBindings(target.deviceGroup, profile)) {
            if (binding.actionId == target.actionId && binding.slot == target.slot) {
                previousChord = binding.triggerCode;
                break;
            }
        }
    }

    // 2) Claim the chord from Windows before anything is persisted. On refusal
    //    the previous registration stays live and nothing is written.
    if (ownsHotkey) {
        QString reason;
        if (!m_hotkeyApply(target.actionId, target.slot, target.triggerCode, &reason)) {
            setRelationNotice(
                QStringLiteral("hotkey_unavailable"),
                reason.isEmpty()
                    ? QStringLiteral("This shortcut is already used by Windows or another application.")
                    : reason);
            return false;
        }
    }

    // 3) Persist: the displaced bindings first, then the new one. Remember what
    //    each displaced key held before the transaction — undoing a write must
    //    put that row back, not delete it: a displaced action that already had
    //    a custom override would otherwise roll back to the shipped default.
    const auto overrideKey = [](const BindingOverrideRow& row) {
        return row.deviceGroup + QLatin1Char('|') + row.deviceProfile + QLatin1Char('|')
             + row.actionId + QLatin1Char('#') + QString::number(row.slot);
    };
    QHash<QString, BindingOverrideRow> before;
    if (!change.conflicts.isEmpty()) {
        for (const BindingOverrideRow& row : m_database->listBindingOverrides())
            before.insert(overrideKey(row), row);
    }
    QVector<BindingOverrideRow> written;
    bool persisted = true;
    for (const auto& conflict : change.conflicts) {
        BindingOverrideRow row{conflict.deviceGroup, profile, conflict.actionId,
                               conflict.slot, {}, conflict.activation, conflict.holdMs, true};
        if (!persist(row)) {
            persisted = false;
            break;
        }
        written.append(row);
    }
    if (persisted) {
        BindingOverrideRow row{target.deviceGroup, profile, target.actionId, target.slot,
                               target.triggerCode, target.activation, target.holdMs, false};
        persisted = persist(row);
    }

    // 4) A failed write rolls the OS back to the chord that was live before, and
    //    undoes the rows that did land, so the three views cannot disagree.
    if (!persisted) {
        for (const auto& row : written) {
            const auto it = before.constFind(overrideKey(row));
            if (it != before.cend())
                persist(*it);
            else
                m_database->clearBindingOverride(row.deviceGroup, row.deviceProfile,
                                                 row.actionId, row.slot);
        }
        if (ownsHotkey) {
            QString reason;
            m_hotkeyApply(target.actionId, target.slot, previousChord, &reason);
        }
        setRelationNotice(QStringLiteral("persistence_error"),
                          QStringLiteral("This shortcut could not be saved. The previous "
                                         "assignment is still active."));
        reloadAndRefresh();
        return false;
    }

    // 5) Only a fully committed change refreshes the editor.
    reloadAndRefresh();
    return true;
}

void BindingEditorModel::clearBinding(const QString& actionId, int slot)
{
    const auto* action = ActionCatalog::find(actionId);
    if (!action || !action->bindable)
        return;
    // The unbound row keeps the slot's gesture. Writing press/0 here (as this
    // did before 0.7.3) silently degraded a tap/hold/double_tap slot to press,
    // so the next capture into it collided with every timed gesture sharing
    // the trigger.
    const BindingResolver::Gesture gesture =
        m_runtime->inheritedGesture(m_deviceGroup, selectedProfile(), actionId, slot);
    BindingOverrideRow row{m_deviceGroup, selectedProfile(), actionId, slot, {},
                           gesture.activation, gesture.holdMs, true};
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
