#include "input/BindingResolver.h"

#include "input/BindingPattern.h"
#include "input/ContextOverrideCatalog.h"
#include "input/ControlId.h"
#include "input/InputDiagnostics.h"
#include "storage/CaptureDatabase.h"

#include <QDebug>
#include <QHash>
#include <QSet>
#include <algorithm>
#include <utility>

namespace {
QString bindingKey(const QString& actionId, int slot)
{
    return actionId + QLatin1Char('#') + QString::number(slot);
}

// The one place a persisted row becomes a live binding, and therefore the one
// place that decides whether it may execute at all. A row that does not form a
// valid pattern — a chord serialization this build does not know, two of the
// same control, a gesture whose parts contradict each other, a value corrupted
// outside the app — is skipped here, so the action falls back to its default
// instead of firing something nobody asked for.
bool validateRow(const BindingOverrideRow& row, QString* error)
{
    // A cleared slot deliberately keeps its gesture and has no trigger, so
    // only the gesture half is meaningful. Validating the trigger here would
    // reject every "unbound" sentinel the editor writes.
    if (row.unbound || row.triggerCode.isEmpty()) {
        const auto gesture = GestureSpec::parse(row.activation, row.tapCount, row.holdMs);
        if (error)
            *error = gesture.error;
        return gesture.ok;
    }

    const auto pattern = BindingPattern::parse(row.deviceGroup, row.triggerCode,
                                               row.activation, row.tapCount, row.holdMs);
    if (error)
        *error = pattern.error;
    return pattern.ok;
}
}

BindingResolver::BindingResolver(CaptureDatabase* database)
    : m_database(database)
{
}

void BindingResolver::setDefaultHoldMs(int milliseconds)
{
    m_defaultHoldMs = qBound(250, milliseconds, 10000);
}

void BindingResolver::setProfileAlias(const QString& profile, const QString& legacyProfile)
{
    setProfileAliases(profile, legacyProfile.isEmpty() ? QStringList{}
                                                        : QStringList{legacyProfile});
}

void BindingResolver::setProfileAliases(const QString& profile,
                                        const QStringList& legacyProfiles)
{
    if (profile.isEmpty())
        return;
    QStringList aliases;
    for (const QString& alias : legacyProfiles) {
        if (!alias.isEmpty() && alias != profile && !aliases.contains(alias))
            aliases.append(alias);
    }
    if (aliases.isEmpty())
        m_profileAliases.remove(profile);
    else
        m_profileAliases.insert(profile, aliases);
}

void BindingResolver::reload()
{
    m_overrides.clear();
    if (!m_database)
        return;
    for (const BindingOverrideRow& row : m_database->listBindingOverrides()) {
        QString error;
        if (!validateRow(row, &error)) {
            const QString subject = QStringLiteral("%1 slot %2 (%3)")
                                        .arg(row.actionId).arg(row.slot).arg(row.deviceGroup);
            qWarning() << "Bindings: skipping stored override" << subject << "—" << error;
            InputDiagnostics::instance().noteRejectedBinding(subject, error);
            continue;
        }
        m_overrides.append({row.deviceGroup, row.deviceProfile, row.actionId,
                            row.slot, row.triggerCode, row.activation,
                            row.holdMs, row.unbound, row.tapCount});
    }
}

// A capture into an empty slot must not invent semantics. The slot's meaning is
// whatever the user or the defaults last gave it, resolved in a fixed order:
// the slot's own override row (a cleared row keeps carrying its old gesture),
// the default binding for that exact slot, another live slot of the same
// action, any default of the same action, and only then plain press. Without
// this, every empty-slot capture landed as "press", which the relation policy
// rightly treats as clashing with any tap/hold on the same trigger — so valid
// assignments were blocked by conflicts that only existed because of the guess.
BindingResolver::Gesture BindingResolver::inheritedGesture(
    const QString& deviceGroup, const QString& deviceProfile,
    const QString& actionId, int slot) const
{
    // 1) The slot's own override row, unbound included — a device-specific row
    //    wins over a group-wide one, same precedence as effectiveBindings().
    //    An unbound press/0 row is the pre-0.7.3 clear sentinel, written when
    //    clearing still erased the gesture; it carries no information, so it
    //    falls through instead of resurrecting "press".
    const Binding* overrideRow = nullptr;
    for (const Binding& binding : m_overrides) {
        if (binding.deviceGroup != deviceGroup || binding.actionId != actionId
            || binding.slot != slot)
            continue;
        if (!binding.deviceProfile.isEmpty() && binding.deviceProfile != deviceProfile)
            continue;
        if (!overrideRow || (overrideRow->deviceProfile.isEmpty()
                             && !binding.deviceProfile.isEmpty()))
            overrideRow = &binding;
    }
    if (overrideRow
        && !(overrideRow->unbound && overrideRow->activation == QLatin1String("press")
             && overrideRow->holdMs == 0))
        return {overrideRow->activation, overrideRow->holdMs, overrideRow->tapCount};

    // 2) The default binding for this exact slot.
    const QVector<Binding> defaults = defaultBindings();
    for (const Binding& binding : defaults) {
        if (binding.deviceGroup == deviceGroup && binding.actionId == actionId
            && binding.slot == slot)
            return {binding.activation, binding.holdMs, binding.tapCount};
    }

    // 3) Another live slot of the same action, lowest slot first.
    const QVector<Binding> effective = effectiveBindings(deviceGroup, deviceProfile);
    const Binding* sibling = nullptr;
    for (const Binding& binding : effective) {
        if (binding.actionId != actionId || binding.slot == slot)
            continue;
        if (!sibling || binding.slot < sibling->slot)
            sibling = &binding;
    }
    if (sibling)
        return {sibling->activation, sibling->holdMs, sibling->tapCount};

    // 4) The action's default semantics: this device group first, then any.
    for (const Binding& binding : defaults) {
        if (binding.deviceGroup == deviceGroup && binding.actionId == actionId)
            return {binding.activation, binding.holdMs, binding.tapCount};
    }
    for (const Binding& binding : defaults) {
        if (binding.actionId == actionId)
            return {binding.activation, binding.holdMs, binding.tapCount};
    }

    // 5) Nothing anywhere has ever given this slot a meaning.
    return {};
}

// Built-in defaults are written in the pattern model: `tap(n)` carries its own
// count, and a `hold()` with no duration stores 0, which means "use whatever
// hold duration is configured". Baking the configured value into every default
// row instead would give the same setting two homes.
QVector<BindingResolver::Binding> BindingResolver::defaultBindings()
{
    using namespace ControlId;
    const auto c = [](const char* action, int slot, const QString& trigger,
                      const GestureSpec& gesture = GestureSpec::press()) {
        return Binding{QStringLiteral("controller"), {}, QString::fromLatin1(action),
                       slot, trigger, gesture.activationCode(), gesture.holdMs, false,
                       gesture.tapCount};
    };
    const auto k = [](const char* action, int slot, const char* chord) {
        return Binding{QStringLiteral("keyboard"), {}, QString::fromLatin1(action),
                       slot, QString::fromLatin1(chord), QStringLiteral("press"), 0, false, 1};
    };
    const auto tap = [](int count) { return GestureSpec::tap(count); };
    // No argument = the configured default hold duration.
    const auto hold = [](int ms = 0) { return GestureSpec::hold(ms); };

    return {
        k("global.toggle_overlay", 1, "Ctrl+Shift+G"),
        k("global.screenshot", 1, "Ctrl+Shift+S"),
        k("global.save_replay", 1, "Ctrl+Shift+E"),

        k("overlay.navigate_up", 1, "Up"),
        k("overlay.navigate_up", 2, "W"),
        k("overlay.navigate_down", 1, "Down"),
        k("overlay.navigate_down", 2, "S"),
        k("overlay.navigate_left", 1, "Left"),
        k("overlay.navigate_left", 2, "A"),
        k("overlay.navigate_right", 1, "Right"),
        k("overlay.navigate_right", 2, "D"),
        k("overlay.confirm", 1, "Return"),
        k("overlay.confirm", 2, "Enter"),
        k("overlay.back", 1, "Esc"),
        k("overlay.back", 2, "Backspace"),
        k("overlay.favorite", 1, "F"),
        k("overlay.menu", 1, "M"),
        k("overlay.sidebar_toggle", 1, "Tab"),
        k("overlay.game_prev", 1, "PgUp"),
        k("overlay.game_next", 1, "PgDown"),

        k("desktop.navigate_up", 1, "Up"),
        k("desktop.navigate_up", 2, "W"),
        k("desktop.navigate_down", 1, "Down"),
        k("desktop.navigate_down", 2, "S"),
        k("desktop.navigate_left", 1, "Left"),
        k("desktop.navigate_left", 2, "A"),
        k("desktop.navigate_right", 1, "Right"),
        k("desktop.navigate_right", 2, "D"),
        k("desktop.confirm", 1, "Return"),
        k("desktop.confirm", 2, "Enter"),
        k("desktop.back", 1, "Esc"),
        k("desktop.back", 2, "Backspace"),
        k("desktop.favorite", 1, "F"),
        k("desktop.menu", 1, "M"),
        k("desktop.tab_prev", 1, "PgUp"),
        k("desktop.tab_next", 1, "PgDown"),

        k("playback.play_pause", 1, "Space"),
        k("playback.play_pause", 2, "Return"),
        k("playback.seek_back", 1, "Left"),
        k("playback.seek_forward", 1, "Right"),
        // S grabs the on-screen clip frame while playback is focused. Only
        // resolves in Playback scope (nothing else binds a bare S there), so it
        // never collides with the global Ctrl+Shift+S screenshot hotkey.
        k("playback.frame_grab", 1, "S"),

        c("global.screenshot", 1, Capture, tap(1)),
        c("global.save_replay", 1, Capture, hold()),
        // Guide carries a tap/hold pair like Share's screenshot/save-replay:
        // a tap toggles the overlay, a 2 s hold summons the desktop window
        // with focus (hold again to return to the game). Tap, not press, so
        // the hold cannot also fire the overlay on the way down — the runtime
        // only suppresses a tap when a hold on the same control has fired.
        c("global.toggle_overlay", 1, Guide, tap(1)),
        c("global.toggle_overlay", 2, Capture, tap(2)),
        c("global.toggle_desktop", 1, Guide, hold(2000)),

        c("overlay.navigate_up", 1, DpadUp),
        c("overlay.navigate_down", 1, DpadDown),
        c("overlay.navigate_left", 1, DpadLeft),
        c("overlay.navigate_right", 1, DpadRight),
        c("overlay.confirm", 1, FaceSouth),
        c("overlay.back", 1, FaceEast),
        c("overlay.favorite", 1, FaceNorth),
        c("overlay.menu", 1, FaceWest),
        c("overlay.sidebar_toggle", 1, Menu),
        c("overlay.game_prev", 1, ShoulderLeft),
        c("overlay.game_next", 1, ShoulderRight),

        c("desktop.navigate_up", 1, DpadUp),
        c("desktop.navigate_down", 1, DpadDown),
        c("desktop.navigate_left", 1, DpadLeft),
        c("desktop.navigate_right", 1, DpadRight),
        // Cross is tap, not press: it shares the button with the bulk-select
        // hold below, and the runtime only suppresses a tap when a hold has
        // already fired. On "press" both would fire on a long press — opening
        // the capture *and* entering bulk mode. Same shape as Share's
        // screenshot-tap / save-replay-hold pair above.
        c("desktop.confirm", 1, FaceSouth, tap(1)),
        c("desktop.back", 1, FaceEast),
        c("desktop.favorite", 1, FaceNorth),
        c("desktop.menu", 1, FaceWest),
        c("desktop.tab_prev", 1, ShoulderLeft),
        c("desktop.tab_next", 1, ShoulderRight),
        // Options opens Settings: in Desktop scope that button was unbound
        // (its only binding is overlay.sidebar_toggle, a different scope).
        c("desktop.settings", 1, Menu),
        c("desktop.zoom_out", 1, TriggerLeft),
        c("desktop.zoom_in", 1, TriggerRight),
        // The right stick scrolls whichever view is showing, like a mouse
        // wheel — the selection stays put. Decoded by the Sony and XInput
        // backends (WinMM has no reliable right-stick axis mapping).
        c("desktop.scroll_up", 1, StickRightUp),
        c("desktop.scroll_down", 1, StickRightDown),
        // Hold Cross for a second to enter bulk selection. Cross keeps its
        // press binding (desktop.confirm) — the runtime only fires the hold
        // once the button has been down that long, so a normal tap still opens
        // the capture.
        c("desktop.bulk_toggle", 1, FaceSouth, hold(1000)),

        c("playback.play_pause", 1, FaceSouth),
        c("playback.seek_back", 1, DpadLeft),
        c("playback.seek_forward", 1, DpadRight),
        // Share (Create) grabs the current clip frame while playback is focused.
        // In every other scope Share tap is global.screenshot. The two do not
        // stack: ContextOverrideCatalog declares frame_grab as an explicit
        // substitution for screenshot on the tap gesture, so a tap during
        // playback produces one capture, not two. Hold (save_replay) and
        // double-tap (toggle_overlay) are different activations and stay live.
        c("playback.frame_grab", 1, Capture, tap(1)),
    };
}

QVector<BindingResolver::Binding> BindingResolver::effectiveBindings(
    const QString& deviceGroup, const QString& deviceProfile) const
{
    QHash<QString, Binding> merged;
    for (const Binding& binding : defaultBindings()) {
        if (binding.deviceGroup == deviceGroup)
            merged.insert(bindingKey(binding.actionId, binding.slot), binding);
    }

    // Precedence, later wins: group-wide < legacy alias (pre-identity
    // "xinput.slotN" rows kept alive for this profile) < device-specific.
    QStringList profiles{QString()};
    if (!deviceProfile.isEmpty()) {
        profiles.append(m_profileAliases.value(deviceProfile));
        profiles.append(deviceProfile);
    }
    for (const QString& profile : profiles) {
        for (const Binding& binding : m_overrides) {
            if (binding.deviceGroup != deviceGroup || binding.deviceProfile != profile)
                continue;
            const auto* action = ActionCatalog::find(binding.actionId);
            if (!action || !action->bindable)
                continue;
            merged.insert(bindingKey(binding.actionId, binding.slot), binding);
        }
    }

    QVector<Binding> result;
    for (const Binding& binding : std::as_const(merged)) {
        if (!binding.unbound && !binding.triggerCode.isEmpty())
            result.append(binding);
    }
    return result;
}

QVector<BindingResolver::Binding> BindingResolver::baselineBindings(
    const QString& deviceGroup, const QString& deviceProfile) const
{
    QHash<QString, Binding> merged;
    for (const Binding& binding : defaultBindings()) {
        if (binding.deviceGroup == deviceGroup)
            merged.insert(bindingKey(binding.actionId, binding.slot), binding);
    }

    // A shared profile compares directly with code-owned defaults. A specific
    // controller compares with everything below its own identity: group-wide
    // rows first, then its legacy slot alias when one exists.
    if (!deviceProfile.isEmpty()) {
        QStringList inheritedProfiles{QString()};
        inheritedProfiles.append(m_profileAliases.value(deviceProfile));
        for (const QString& profile : inheritedProfiles) {
            for (const Binding& binding : m_overrides) {
                if (binding.deviceGroup != deviceGroup || binding.deviceProfile != profile)
                    continue;
                const auto* action = ActionCatalog::find(binding.actionId);
                if (!action || !action->bindable)
                    continue;
                merged.insert(bindingKey(binding.actionId, binding.slot), binding);
            }
        }
    }

    QVector<Binding> result;
    for (const Binding& binding : std::as_const(merged)) {
        if (!binding.unbound && !binding.triggerCode.isEmpty())
            result.append(binding);
    }
    return result;
}

QVector<BindingResolver::Binding> BindingResolver::matching(
    const QString& deviceGroup, const QString& deviceProfile,
    const QString& triggerCode, const GestureSpec& gesture,
    ActionCatalog::Scope primaryScope, ActionCatalog::Scope fallbackScope) const
{
    QVector<Binding> globals;
    QVector<Binding> primary;
    QVector<Binding> fallback;
    for (const Binding& binding : effectiveBindings(deviceGroup, deviceProfile)) {
        // Kind AND tap count: "tap" alone would let a double-tap binding answer
        // a single tap. Hold durations are deliberately not compared — the
        // runtime decides which hold is due from elapsed time.
        const GestureSpec candidate = binding.gesture();
        if (binding.triggerCode != triggerCode || candidate.kind != gesture.kind
            || candidate.tapCount != gesture.tapCount)
            continue;
        const ActionCatalog::Action* action = ActionCatalog::find(binding.actionId);
        if (!action)
            continue;
        if (action->scope == ActionCatalog::Scope::Global)
            globals.append(binding);
        else if (action->scope == primaryScope)
            primary.append(binding);
        else if (action->scope == fallbackScope)
            fallback.append(binding);
    }
    const QVector<Binding>& contextual = primary.isEmpty() ? fallback : primary;

    // Global bindings normally fire alongside the contextual ones, so a Guide
    // press still toggles the overlay while a clip is focused. Drop only the
    // pairs ContextOverrideCatalog declares as deliberate substitutions —
    // never a blanket "contextual shadows Global" rule, which would hide
    // user-created overlaps the binding editor is supposed to surface.
    QVector<Binding> result;
    for (const Binding& global : globals) {
        const bool shadowed = std::any_of(
            contextual.cbegin(), contextual.cend(), [&](const Binding& active) {
                return ContextOverrideCatalog::shadows(active.actionId, global.actionId,
                                                       gesture);
            });
        if (!shadowed)
            result.append(global);
    }
    result += contextual;
    return result;
}
