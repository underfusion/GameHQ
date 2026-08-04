#include "input/BindingRuntime.h"

#include "input/InputDiagnostics.h"

#include <QSet>
#include <algorithm>
#include <utility>

BindingRuntime::BindingRuntime(CaptureDatabase* database, QObject* parent)
    : QObject(parent)
    , m_resolver(database)
    , m_recognizer(this)
{
    m_recognizer.setFactsProvider(
        [this](const InputPatternRecognizer::Context& context, const QString& control) {
            return factsFor(context, control);
        });
    connect(&m_recognizer, &InputPatternRecognizer::patternNote, this,
            [](const QString& detail) { InputDiagnostics::instance().notePattern(detail); });
    connect(&m_recognizer, &InputPatternRecognizer::recognized, this,
            [this](const InputPatternRecognizer::Context& context, const TriggerSpec& trigger,
                   const GestureSpec& gesture) { dispatch(context, trigger, gesture); });
}

BindingRuntime::~BindingRuntime() = default;

void BindingRuntime::setDefaultHoldMs(int milliseconds)
{
    m_resolver.setDefaultHoldMs(milliseconds);
    InputPatternRecognizer::Timing timing = m_recognizer.timing();
    timing.defaultHoldMs = m_resolver.defaultHoldMs();
    m_recognizer.setTiming(timing);
}

void BindingRuntime::setTiming(const InputPatternRecognizer::Timing& timing)
{
    m_recognizer.setTiming(timing);
}

InputPatternRecognizer::Timing BindingRuntime::timing() const
{
    return m_recognizer.timing();
}

void BindingRuntime::reload()
{
    cancelAll();
    m_resolver.reload();
    m_relations.clear();
    // The binding table the recognizer snapshotted no longer exists.
    m_recognizer.invalidate();
    publishBoundPatterns();
}

const QVector<BindingRuntime::Relation>& BindingRuntime::relations(
    const QString& deviceGroup, const QString& deviceProfile) const
{
    const QString key = deviceGroup + QLatin1Char('\x1f') + deviceProfile;
    const auto cached = m_relations.constFind(key);
    if (cached != m_relations.cend())
        return *cached;

    const QVector<BindingResolver::Binding> bindings =
        m_resolver.effectiveBindings(deviceGroup, deviceProfile);
    QVector<Relation> classified;
    for (int i = 0; i < bindings.size(); ++i) {
        for (int j = i + 1; j < bindings.size(); ++j) {
            const BindingRelation::Kind kind =
                BindingRelation::classify(bindings.at(i), bindings.at(j));
            if (kind != BindingRelation::Kind::None)
                classified.append({bindings.at(i), bindings.at(j), kind});
        }
    }
    return *m_relations.insert(key, classified);
}

QVector<BindingResolver::Binding> BindingRuntime::effectiveBindings(
    const QString& deviceGroup, const QString& deviceProfile) const
{
    return m_resolver.effectiveBindings(deviceGroup, deviceProfile);
}

QVector<BindingResolver::Binding> BindingRuntime::baselineBindings(
    const QString& deviceGroup, const QString& deviceProfile) const
{
    return m_resolver.baselineBindings(deviceGroup, deviceProfile);
}

BindingResolver::Gesture BindingRuntime::inheritedGesture(
    const QString& deviceGroup, const QString& deviceProfile,
    const QString& actionId, int slot) const
{
    return m_resolver.inheritedGesture(deviceGroup, deviceProfile, actionId, slot);
}

void BindingRuntime::setProfileAlias(const QString& profile, const QString& legacyProfile)
{
    m_resolver.setProfileAlias(profile, legacyProfile);
    // The alias changes the effective view, so cached pair classifications
    // for the affected profile are stale.
    m_relations.clear();
    m_recognizer.invalidate();
}

void BindingRuntime::setProfileAliases(const QString& profile,
                                       const QStringList& legacyProfiles)
{
    m_resolver.setProfileAliases(profile, legacyProfiles);
    m_relations.clear();
    m_recognizer.invalidate();
}

// A Hold binding that stores 0 means "however long the user configured".
// Built-in hold defaults do exactly that, so the setting has one home instead
// of being copied into every default row at construction time.
int BindingRuntime::holdThreshold(const BindingResolver::Binding& binding) const
{
    return binding.holdMs > 0 ? binding.holdMs : m_resolver.defaultHoldMs();
}

// Everything the recognizer needs to know about one control, read off the
// effective binding table once per pattern. Scope filtering matches dispatch:
// a Hold bound only in the Overlay scope must not make the button wait while
// the gallery is focused.
InputPatternRecognizer::TriggerFacts BindingRuntime::factsFor(
    const InputPatternRecognizer::Context& context, const QString& control) const
{
    InputPatternRecognizer::TriggerFacts facts;
    QSet<int> thresholds;
    for (const auto& binding : m_resolver.effectiveBindings(context.deviceGroup,
                                                            context.deviceProfile)) {
        const ActionCatalog::Action* action = ActionCatalog::find(binding.actionId);
        if (!action)
            continue;
        const bool inContext = action->scope == ActionCatalog::Scope::Global
                            || action->scope == context.primaryScope
                            || action->scope == context.fallbackScope;
        if (!inContext)
            continue;

        const TriggerSpec trigger = binding.trigger();
        if (trigger.isChord()) {
            // Only the control that *starts* the chord opens a candidate.
            if (trigger.firstControl() == control
                && !facts.chordPartners.contains(trigger.secondControl()))
                facts.chordPartners.append(trigger.secondControl());
            continue;
        }
        if (trigger.firstControl() != control)
            continue;

        const GestureSpec gesture = binding.gesture();
        switch (gesture.kind) {
        case GestureSpec::Kind::Press:
            facts.hasPress = true;
            break;
        case GestureSpec::Kind::Tap:
            facts.tapCountMask |= 1 << gesture.tapCount;
            break;
        case GestureSpec::Kind::Hold:
            thresholds.insert(holdThreshold(binding));
            break;
        }
    }
    facts.holdThresholdsMs = QVector<int>(thresholds.cbegin(), thresholds.cend());
    std::sort(facts.holdThresholdsMs.begin(), facts.holdThresholdsMs.end());
    return facts;
}

// The effective controller assignments in canonical form, so a diagnostics
// paste shows what was bound rather than only what happened.
void BindingRuntime::publishBoundPatterns()
{
    QStringList patterns;
    for (const auto& binding : m_resolver.effectiveBindings(QStringLiteral("controller"))) {
        patterns << QStringLiteral("%1 %2 -> %3 (slot %4)")
                        .arg(binding.trigger().serialize(), binding.gesture().label(),
                             binding.actionId)
                        .arg(binding.slot);
    }
    patterns.sort();
    InputDiagnostics::instance().setBoundPatterns(patterns);
}

void BindingRuntime::dispatch(const InputPatternRecognizer::Context& context,
                              const TriggerSpec& trigger, const GestureSpec& gesture)
{
    const QString triggerCode = trigger.serialize();
    const auto bindings = m_resolver.matching(context.deviceGroup, context.deviceProfile,
                                              triggerCode, gesture, context.primaryScope,
                                              context.fallbackScope);
    QSet<QString> emitted;
    for (const auto& binding : bindings) {
        // Holds are matched by kind alone in the resolver, because a binding's
        // stored 0 has to be resolved against the configured default before it
        // can be compared. Two holds on one button (open at 1 s, bulk select at
        // 2 s) therefore separate here, on the threshold that actually elapsed.
        if (gesture.kind == GestureSpec::Kind::Hold
            && holdThreshold(binding) != gesture.holdMs)
            continue;
        if (emitted.contains(binding.actionId))
            continue;
        emitted.insert(binding.actionId);
        emit actionTriggered(binding.actionId, triggerCode);
        InputDiagnostics::instance().notePattern(
            QStringLiteral("%1 %2 -> %3").arg(triggerCode, gesture.label(), binding.actionId));
    }
}

bool BindingRuntime::press(const QString& group, const QString& profile,
                           const QString& trigger, ActionCatalog::Scope primary,
                           ActionCatalog::Scope fallback)
{
    const InputPatternRecognizer::Context context{group, profile, primary, fallback};
    m_pressContexts.insert(group + QChar(0x1f) + profile + QChar(0x1f) + trigger, context);
    return m_recognizer.press(context, trigger);
}

bool BindingRuntime::release(const QString& group, const QString& profile,
                             const QString& trigger)
{
    // Release carries no scope of its own — the pattern belongs to the context
    // its press started in, which is also what the recognizer keyed its state
    // by. Looking it up here keeps every caller from having to remember it.
    const auto it = m_pressContexts.constFind(group + QChar(0x1f) + profile
                                              + QChar(0x1f) + trigger);
    if (it == m_pressContexts.cend())
        return false;
    return m_recognizer.release(*it, trigger);
}

void BindingRuntime::cancelAll()
{
    m_recognizer.invalidate();
}
