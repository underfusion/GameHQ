#include "input/BindingRuntime.h"

#include <QElapsedTimer>
#include <QSet>
#include <QTimer>
#include <utility>

struct BindingRuntime::GestureState
{
    QString key;
    QString triggerCode;
    QVector<BindingResolver::Binding> taps;
    QVector<BindingResolver::Binding> holds;
    QVector<BindingResolver::Binding> doubleTaps;
    QSet<QString> firedHolds;
    QElapsedTimer elapsed;
    QTimer* holdTimer = nullptr;
    QTimer* tapTimer = nullptr;
    bool down = false;
    bool awaitingSecondTap = false;
    bool secondTap = false;

    ~GestureState()
    {
        delete holdTimer;
        delete tapTimer;
    }
};

BindingRuntime::BindingRuntime(CaptureDatabase* database, QObject* parent)
    : QObject(parent)
    , m_resolver(database)
{
}

BindingRuntime::~BindingRuntime()
{
    for (GestureState* state : std::as_const(m_states))
        delete state;
}

void BindingRuntime::setDefaultHoldMs(int milliseconds)
{
    m_resolver.setDefaultHoldMs(milliseconds);
}

void BindingRuntime::reload()
{
    cancelAll();
    m_resolver.reload();
}

QVector<BindingResolver::Binding> BindingRuntime::effectiveBindings(
    const QString& deviceGroup, const QString& deviceProfile) const
{
    return m_resolver.effectiveBindings(deviceGroup, deviceProfile);
}

QString BindingRuntime::stateKey(const QString& group, const QString& profile,
                                 const QString& trigger) const
{
    return group + QChar(0x1f) + profile + QChar(0x1f) + trigger;
}

void BindingRuntime::emitBindings(const QVector<BindingResolver::Binding>& bindings,
                                  const QString& triggerCode)
{
    QSet<QString> emitted;
    for (const auto& binding : bindings) {
        if (emitted.contains(binding.actionId))
            continue;
        emitted.insert(binding.actionId);
        emit actionTriggered(binding.actionId, triggerCode);
    }
}

bool BindingRuntime::press(const QString& group, const QString& profile,
                           const QString& trigger, ActionCatalog::Scope primary,
                           ActionCatalog::Scope fallback)
{
    const auto presses = m_resolver.matching(group, profile, trigger, QStringLiteral("press"),
                                             primary, fallback);
    emitBindings(presses, trigger);

    const auto taps = m_resolver.matching(group, profile, trigger, QStringLiteral("tap"),
                                          primary, fallback);
    const auto holds = m_resolver.matching(group, profile, trigger, QStringLiteral("hold"),
                                           primary, fallback);
    const auto doubles = m_resolver.matching(group, profile, trigger, QStringLiteral("double_tap"),
                                             primary, fallback);
    if (taps.isEmpty() && holds.isEmpty() && doubles.isEmpty())
        return !presses.isEmpty();

    const QString key = stateKey(group, profile, trigger);
    GestureState*& state = m_states[key];
    if (!state) {
        state = new GestureState;
        state->key = key;
        state->holdTimer = new QTimer;
        state->holdTimer->setSingleShot(true);
        state->tapTimer = new QTimer;
        state->tapTimer->setSingleShot(true);
        connect(state->holdTimer, &QTimer::timeout, this, [this, state] {
            const int elapsed = static_cast<int>(state->elapsed.elapsed());
            QVector<BindingResolver::Binding> due;
            for (const auto& binding : state->holds) {
                const int threshold = binding.holdMs > 0 ? binding.holdMs : 500;
                if (elapsed >= threshold && !state->firedHolds.contains(binding.actionId)) {
                    state->firedHolds.insert(binding.actionId);
                    due.append(binding);
                }
            }
            emitBindings(due, state->triggerCode);
            scheduleNextHold(state);
        });
        connect(state->tapTimer, &QTimer::timeout, this, [this, state] {
            emitBindings(state->taps, state->triggerCode);
            reset(state);
        });
    }

    state->triggerCode = trigger;
    state->taps = taps;
    state->holds = holds;
    state->doubleTaps = doubles;
    state->firedHolds.clear();
    state->secondTap = state->awaitingSecondTap;
    if (state->secondTap) {
        state->tapTimer->stop();
        state->awaitingSecondTap = false;
    }
    state->down = true;
    state->elapsed.restart();
    scheduleNextHold(state);
    return true;
}

void BindingRuntime::scheduleNextHold(GestureState* state)
{
    state->holdTimer->stop();
    if (!state->down)
        return;
    int nextMs = -1;
    const int elapsed = static_cast<int>(state->elapsed.elapsed());
    for (const auto& binding : state->holds) {
        if (state->firedHolds.contains(binding.actionId))
            continue;
        const int threshold = binding.holdMs > 0 ? binding.holdMs : 500;
        const int remaining = qMax(1, threshold - elapsed);
        nextMs = nextMs < 0 ? remaining : qMin(nextMs, remaining);
    }
    if (nextMs > 0)
        state->holdTimer->start(nextMs);
}

bool BindingRuntime::release(const QString& group, const QString& profile,
                             const QString& trigger)
{
    GestureState* state = m_states.value(stateKey(group, profile, trigger));
    if (!state || !state->down)
        return false;
    state->down = false;
    state->holdTimer->stop();

    if (!state->firedHolds.isEmpty()) {
        reset(state);
        return true;
    }
    if (state->secondTap) {
        emitBindings(state->doubleTaps, trigger);
        reset(state);
        return true;
    }
    if (!state->doubleTaps.isEmpty()) {
        state->awaitingSecondTap = true;
        state->tapTimer->start(300);
        return true;
    }
    emitBindings(state->taps, trigger);
    reset(state);
    return true;
}

void BindingRuntime::reset(GestureState* state)
{
    state->holdTimer->stop();
    state->tapTimer->stop();
    state->down = false;
    state->awaitingSecondTap = false;
    state->secondTap = false;
    state->firedHolds.clear();
}

void BindingRuntime::cancelAll()
{
    for (GestureState* state : std::as_const(m_states))
        reset(state);
}
