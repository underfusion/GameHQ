#pragma once
#include "input/Gamepad.h"

class QTimer;

// Fallback for DSX Xbox mode and real Xbox pads. Xbox controllers do not
// expose a Share/Create button through standard XInput, so Back maps to
// Share; the Guide button (XInputGetStateEx, xinput1_4 ordinal 100) maps to
// PS when the loaded XInput DLL provides it.
//
// XInputGetState on an EMPTY slot can stall for milliseconds (documented
// XInput pitfall), so empty slots are never polled continuously: they are
// probed once at start, on rescan() — driven by the Raw Input backend's
// device-topology hint — and by a slow safety-net timer. Connected slots
// are polled fast; a slot that stops answering emits releases for its held
// buttons before reporting the disconnect.
class XInputDevice : public Gamepad
{
    Q_OBJECT
public:
    explicit XInputDevice(QObject* parent = nullptr);
    ~XInputDevice() override;

    bool start() override;
    ControlId::DeviceProfile profile() const override;

public slots:
    void rescan();   // probe empty slots for newly arrived pads

private:
    void poll();     // fast-poll connected slots only
    void setSlotState(int slot, quint32 buttons, bool connected);
    int connectedCount() const;

    QTimer* m_pollTimer = nullptr;     // runs only while a slot is connected
    QTimer* m_rescanTimer = nullptr;   // slow safety net for missed arrivals
    void* m_library = nullptr;
    using XInputGetStateFn = unsigned long(__stdcall*)(unsigned long, void*);
    XInputGetStateFn m_getState = nullptr;   // XInputGetStateEx when available
    quint32 m_prevButtons[4] = {};
    bool m_connected[4] = {};
    bool m_anyConnected = false;       // aggregate, as last emitted
};
