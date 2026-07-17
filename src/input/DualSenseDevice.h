#pragma once
#include "input/Gamepad.h"

#include <QElapsedTimer>
#include <QHash>
#include <QSet>
#include <QString>
#include <QtGlobal>

class QTimer;

// Sony pad reader over Win32 Raw Input. Handles DualSense, DualSense Edge,
// DS4, and DSX/ViGEm virtual DS4 reports.
//
// A message-only window receives WM_INPUT with RIDEV_INPUTSINK so pad input
// arrives even while a game has focus, and WM_INPUT_DEVICE_CHANGE via
// RIDEV_DEVNOTIFY for hot-plug. Every supported device is tracked with its
// OWN state keyed by Raw Input handle (handles change on every reconnect),
// so two pads reporting at once — e.g. a real DualSense next to DSX's
// virtual DS4 — can't corrupt each other's edge detection. One device is
// "active" at a time; if it is removed or goes silent while another tracked
// pad shows real input, the active role fails over immediately instead of
// waiting out the disconnect debounce.
//
// USB (report 0x01) and Bluetooth (report 0x31, payload shifted +2) layouts
// are both parsed. HID collections whose path contains "IG_" are XInput
// devices and are left to the XInput backend. See docs/controller-input.md.
// Win32 lives in the .cpp only.
class DualSenseDevice : public Gamepad
{
    Q_OBJECT
public:
    explicit DualSenseDevice(QObject* parent = nullptr);
    ~DualSenseDevice() override;

    bool start() override;
    ControlId::DeviceProfile profile() const override;

    // Called from the window procedure — not for general use.
    void onRawInput(void* hRawInput);
    void onDeviceChange(bool arrived, void* deviceHandle);

signals:
    // Debounced hint that the HID device topology changed (any arrival or
    // removal, including XInput/unsupported devices — Windows re-enumerates
    // the whole tree in bursts). InputEngine forwards it to the XInput and
    // WinMM backends so they can rescan on events instead of continuously
    // polling empty slots (XInputGetState on an empty slot can stall for
    // milliseconds — see docs/controller-input.md).
    void deviceTopologyChanged();

private:
    struct DeviceState {
        int layout = 0;             // ReportLayout (DualSense / DS4)
        quint32 vendorId = 0;
        quint32 productId = 0;
        QString path;               // RIDI_DEVICENAME — stable device identity
        quint32 buttons = 0;        // last parsed button+stick bitmask
        quint32 stick = 0;          // stick-derived direction bits (hysteresis)
        qint64 lastReportMs = 0;    // m_clock timestamp of the last report
        bool reported = false;      // produced at least one valid report
    };

    bool registerRawInput(bool remove = false);
    DeviceState* probeDevice(void* handle);   // query + track a handle; null if unsupported
    void removeDevice(void* handle);
    void reconcileDevices();                  // debounced full-list sync (prune stale handles)
    void failoverOrScheduleDisconnect();
    void finishDisconnect();
    void parseReport(void* handle, DeviceState& st, const unsigned char* data, int len);
    // parseReport stages, in call order. The decoders are pure (static);
    // routeReport owns the active-pad selection/steal side effects.
    static int buttonBlockBase(unsigned char reportId, bool ds4, int len);
    static quint32 decodeButtons(const unsigned char* d, int base);
    static quint32 decodeStickNav(const DeviceState& st, const unsigned char* d, int base, int len);
    void routeReport(void* handle, const DeviceState& st, quint32 s, bool changed,
                     unsigned char reportId, const unsigned char* d, int len);
    void emitEdges(quint32 buttons);

    void* m_hwnd = nullptr;                  // HWND of the message-only window
    QHash<void*, DeviceState> m_devices;     // tracked Sony/DS4 devices by Raw Input handle
    QSet<QString> m_loggedIgnored;           // device paths already logged as ignored
    void* m_activeHandle = nullptr;          // device currently driving input (or null)
    QTimer* m_disconnectTimer = nullptr;
    QTimer* m_reconcileTimer = nullptr;
    QTimer* m_topologyTimer = nullptr;
    QElapsedTimer m_clock;
    quint32 m_emittedButtons = 0;            // bitmask InputEngine has seen so far
    bool m_connectedState = false;           // connected(bool) as last emitted
    bool m_sawInput = false;                 // first WM_INPUT diagnostic logged?
};
