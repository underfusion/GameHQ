#pragma once

#include "input/ControlId.h"

#include <QMetaType>
#include <QString>
#include <QStringList>

// Advanced binding patterns, version 1 — the MODEL only.
//
// A binding is a TRIGGER (what has to be pressed) plus a GESTURE (how it is
// pressed). Until now both were flattened into one control id, an activation
// string and a hold duration, which cannot express "hold View, then press
// Guide" or "tap three times". This file introduces the two specs, their
// canonical serialization and the validation that guards the parse boundary.
//
// It deliberately contains no state machine and no timers: recognizing a chord
// or counting taps at runtime is the InputPatternRecognizer's job. Everything
// here is pure, allocation-light and testable without a device or a database.
//
// SERIALIZATION CONTRACT (frozen)
//   Single trigger      -> the control id verbatim ("gamepad.capture",
//                          "Ctrl+Shift+S", "gamepad.button.7"). Every value
//                          written before patterns existed therefore parses
//                          unchanged; nothing needs rewriting on disk.
//   Ordered two-button  -> "chord:v1:<first>><second>", e.g.
//                          "chord:v1:gamepad.capture>gamepad.guide".
//   Anything else under the "chord:" namespace — an unknown version such as
//   "chord:v2:...", a truncated payload, three controls — FAILS CLOSED. A
//   version this build does not understand must never be guessed at and must
//   never execute an action.
//
// The gesture keeps living in the activation / hold_ms columns and gains an
// explicit tap count; the column that persists it arrives with the schema
// migration. Tap x1 and Tap x2 still spell themselves "tap" and "double_tap"
// so an older row round-trips, but Tap x3 has no legacy spelling on purpose —
// hasLegacyActivation() is how a caller finds out before writing.

struct GestureSpec
{
    enum class Kind {
        Press, // fires on the down edge, before any timing is known
        Tap,   // exactly tapCount complete press/release cycles
        Hold   // held down for holdMs (0 = use the configured default)
    };

    Kind kind = Kind::Press;
    int tapCount = 1;
    int holdMs = 0;

    // Version 1 recognizes single, double and triple taps. Above that the
    // timing windows stack up past what a user can reliably produce.
    static constexpr int kMaxTapCount = 3;

    static GestureSpec press();
    static GestureSpec tap(int count);
    static GestureSpec hold(int milliseconds);

    // Frozen rules: Press => tapCount 1, holdMs 0. Tap => tapCount 1..3,
    // holdMs 0. Hold => tapCount 1, holdMs >= 0.
    bool isValid(QString* error = nullptr) const;

    // Canonical token for the activation column: "press", "tap" or "hold".
    // The tap count travels beside it, never inside it.
    QString activationCode() const;

    // The pre-pattern spelling the current schema's CHECK constraint accepts.
    // Tap x3 has none — persisting it needs the tap_count column, so callers
    // that still write the old shape must check this first instead of silently
    // degrading a triple tap to a double one.
    bool hasLegacyActivation() const;
    QString legacyActivationCode() const;

    // Human-facing, family-independent: "Press", "Tap", "Double tap", ...
    QString label() const;

    // Defined below: a result cannot hold the spec it belongs to until that
    // spec is a complete type.
    struct ParseResult;

    // Reads a persisted gesture. `activation` accepts the canonical tokens and
    // the legacy ones; an empty string means "press" (rows written before
    // activations existed). `tapCount` is 1 for any row from a schema without
    // the column, so "double_tap" accepts 1 (unset) or 2 and yields Tap x2.
    // Any unknown activation fails closed.
    static ParseResult parse(const QString& activation, int tapCount, int holdMs);

    bool operator==(const GestureSpec& other) const = default;
};

struct GestureSpec::ParseResult
{
    bool ok = false;
    GestureSpec gesture;
    QString error;
};

struct TriggerSpec
{
    enum class Kind {
        Single,      // one control
        OrderedChord // hold controls[0], then press controls[1]
    };

    Kind kind = Kind::Single;
    QStringList controls;

    static TriggerSpec single(const QString& control);
    static TriggerSpec orderedChord(const QString& first, const QString& second);

    bool isChord() const { return kind == Kind::OrderedChord; }
    QString firstControl() const;
    QString secondControl() const;

    // Frozen rules: Single = exactly one non-empty control that does not sit in
    // the reserved "chord:" namespace. OrderedChord = exactly two DIFFERENT
    // canonical controller controls — an ordered chord over an opaque keyboard
    // code or an unknown id cannot be recognized, so it is rejected here rather
    // than half-working at runtime.
    bool isValid(QString* error = nullptr) const;

    QString serialize() const;

    // Display form for the editor: "Share" or "Share + PS". The instruction
    // ("Hold Share, then press PS") is UI copy and lives with the editor.
    QString label(ControlId::ControllerFamily family) const;

    struct ParseResult;

    static ParseResult parse(const QString& triggerCode);

    // "chord:" — the whole namespace is reserved, so a future version can be
    // recognized as such instead of being mistaken for a control id.
    static QString chordNamespace();
    static QString chordPrefixV1();

    bool operator==(const TriggerSpec& other) const = default;
};

struct TriggerSpec::ParseResult
{
    bool ok = false;
    TriggerSpec trigger;
    QString error;
};

// Trigger plus gesture, with the cross-cutting rules that need both.
struct BindingPattern
{
    TriggerSpec trigger;
    GestureSpec gesture;

    struct ParseResult;

    // The single parse boundary for a persisted binding row. `deviceGroup`
    // matters because chords are controller-only in version 1: keyboard slots
    // keep their modifier+key model, where a chord would collide with the
    // modifier semantics the OS already applies.
    static ParseResult parse(const QString& deviceGroup, const QString& triggerCode,
                             const QString& activation, int tapCount, int holdMs);

    bool isValid(const QString& deviceGroup, QString* error = nullptr) const;

    bool operator==(const BindingPattern& other) const = default;
};

struct BindingPattern::ParseResult
{
    bool ok = false;
    BindingPattern pattern;
    QString error;
};

// Queued signals and QSignalSpy carry these by value.
Q_DECLARE_METATYPE(GestureSpec)
Q_DECLARE_METATYPE(TriggerSpec)
