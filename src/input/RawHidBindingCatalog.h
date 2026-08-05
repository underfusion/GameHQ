#pragma once

#include <QStringList>

class BindingRuntime;
class CaptureDatabase;

namespace ModernInput {

// Collects every persisted Raw HID control that must keep the selective
// producer awake. Shared effective bindings are combined with every stored
// controller profile and both controls of an ordered chord.
QStringList persistedRawHidControls(const BindingRuntime& runtime,
                                    const CaptureDatabase* database);

} // namespace ModernInput
