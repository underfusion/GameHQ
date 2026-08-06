#pragma once

// Read-only probe for Windows HDR (Advanced Color) state and encoder support.
// The report also describes the configured capture policy; actual FP16 support
// is confirmed when FramePumpService arms on a concrete game window.

#include <windows.h>

#include <QRect>
#include <QString>
#include <QStringList>
#include <QVector>

namespace capture {

// One desktop output as DXGI describes it right now. HDR is a per-monitor,
// user-toggleable runtime state, so this is a snapshot, never a cached fact.
struct HdrOutputInfo
{
    QString deviceName;                     // \\.\DISPLAY1
    QRect desktopRect;
    bool valid = false;                     // false when the probe itself failed
    bool hdrActive = false;                 // DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020
    unsigned bitsPerColor = 0;
    float minLuminanceNits = 0.0f;
    float maxLuminanceNits = 0.0f;
    float maxFullFrameLuminanceNits = 0.0f;
    float sdrWhiteLevelNits = 0.0f;         // Windows "SDR content brightness" on HDR
    int advancedColorMode = -1;             // -1 unknown, 0 SDR, 1 WCG, 2 HDR

    QString describe() const;
};

// Whole-machine snapshot: every output plus what the capture stack can do with
// HDR content today.
struct HdrReport
{
    QVector<HdrOutputInfo> outputs;
    bool anyHdrActive = false;
    bool hevcMain10Encoder = false;
    QString captureFormat;                  // what the frame pump actually requests
    QString screenshotSupport;
    QString videoEncoder;
    QString activeFallback;

    // Human-readable lines for the log and the diagnostic summary.
    QStringList summaryLines() const;
};

// All queries are synchronous COM calls that create and release their own DXGI
// factory, so they are safe to call from any thread and from any apartment.
namespace HdrCapabilities {

HdrReport query(bool experimentalHdrEnabled);

// Re-enumerates only display state while preserving the encoder capability
// from a previous full query. Intended for the runtime HDR toggle watcher.
HdrReport refreshDisplayState(const HdrReport& previous,
                              bool experimentalHdrEnabled);

// Compares only fields that can change with display topology or Windows HDR.
// Encoder and configured capture-policy descriptions are intentionally ignored.
bool sameDisplayState(const HdrReport& left, const HdrReport& right);

// Output containing the given monitor/window; an invalid HdrOutputInfo when the
// monitor cannot be matched (remote session, output removed mid-move).
HdrOutputInfo forMonitor(HMONITOR monitor);
HdrOutputInfo forWindow(HWND hwnd);

// Whether a Media Foundation HEVC encoder that accepts Main10 is installed.
// Probed by actually offering the MFT a Main10 output type — the mere presence
// of an HEVC encoder says nothing about 10-bit support.
bool hevcMain10Supported();

// Log the full report once, at the given prefix. Used at startup and whenever
// the display topology changes.
void logReport(const HdrReport& report);

} // namespace HdrCapabilities
} // namespace capture
