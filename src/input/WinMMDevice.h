#pragma once
#include "input/Gamepad.h"

#include <QSet>
#include <QStringList>

#include <climits>
#include <windows.h>

class QTimer;

// Last-resort fallback using the legacy Windows multimedia joystick API
// (joyGetPosEx). Some virtual controllers (DSX, ViGEm, etc.) that do not
// appear through Raw Input or XInput still enumerate as a standard Windows
// joystick.
//
// While disconnected, slots are scanned only on rescan() — driven by the
// Raw Input backend's device-topology hint — plus a slow safety-net timer,
// because probing all 16 empty slots at poll rate is wasted work. While
// connected, the active slot is polled fast; on unplug the held buttons are
// released before the disconnect is reported, then arrival scanning resumes.
class WinMMDevice : public Gamepad
{
    Q_OBJECT
public:
    explicit WinMMDevice(QObject* parent = nullptr);
    ~WinMMDevice() override;

    bool start() override;
    ControlId::DeviceProfile profile() const override;

public slots:
    void rescan();   // scan slots for a newly arrived joystick

public:
    // "vvvv:pppp" identities of the XInput-class (IG_) devices Raw Input
    // currently sees. Windows also exposes every XInput pad through this
    // legacy joystick API, so a WinMM endpoint matching one of these is the
    // same physical device the XInput backend already owns — enumerating it
    // here would create a second event stream for every press. Only strong
    // identity matches are skipped; legacy-only pads keep working.
    void setXInputClassIdentities(const QStringList& identities);

private:
    bool isXInputBacked(quint32 vendorId, quint32 productId) const;

    void poll();
    void disconnectActive();
    void emitEdges(quint32 buttons);

    QTimer* m_pollTimer = nullptr;     // runs only while connected
    QTimer* m_rescanTimer = nullptr;   // slow safety net while disconnected
    quint32 m_prevButtons = 0;
    bool m_connected = false;
    bool m_ds4Layout = false;   // Sony button order (Share=8, PS=12) vs Xbox
    UINT m_activeId = UINT_MAX;
    quint32 m_vendorId = 0;
    quint32 m_productId = 0;
    QSet<QString> m_xinputClassIdentities;
};
