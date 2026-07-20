#include "capture/HdrCapabilities.h"

#include "capture/CaptureUtil.h"

#include <codecapi.h>
#include <dxgi1_6.h>
#include <mfapi.h>
#include <mferror.h>
#include <mfidl.h>

#include <QDebug>

namespace capture {
namespace {

constexpr const char* kTag = "Hdr";

// Small RAII release so every early return still frees the COM pointer.
template <class T>
struct ComPtr
{
    T* p = nullptr;
    ~ComPtr() { if (p) p->Release(); }
    T** put() { return &p; }
    T* operator->() const { return p; }
    explicit operator bool() const { return p != nullptr; }
};

HdrOutputInfo describeOutput(IDXGIOutput* output)
{
    HdrOutputInfo info;
    ComPtr<IDXGIOutput6> output6;
    if (FAILED(output->QueryInterface(__uuidof(IDXGIOutput6),
                                      reinterpret_cast<void**>(output6.put()))))
        return info;   // pre-1703 DXGI: no Advanced Color information at all

    DXGI_OUTPUT_DESC1 desc{};
    if (!CaptureUtil::ok(kTag, "IDXGIOutput6::GetDesc1", output6->GetDesc1(&desc)))
        return info;

    info.valid = true;
    info.deviceName = QString::fromWCharArray(desc.DeviceName);
    info.desktopRect = QRect(desc.DesktopCoordinates.left, desc.DesktopCoordinates.top,
                             desc.DesktopCoordinates.right - desc.DesktopCoordinates.left,
                             desc.DesktopCoordinates.bottom - desc.DesktopCoordinates.top);
    // ST.2084 (PQ) in BT.2020 is the only colour space Windows reports while the
    // "Use HDR" desktop toggle is on; everything else means the desktop is SDR.
    info.hdrActive = desc.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;
    info.bitsPerColor = desc.BitsPerColor;
    info.minLuminanceNits = desc.MinLuminance;
    info.maxLuminanceNits = desc.MaxLuminance;
    info.maxFullFrameLuminanceNits = desc.MaxFullFrameLuminance;
    return info;
}

// Walk every adapter's outputs, calling visit() until it returns false.
template <class Fn>
void forEachOutput(Fn visit)
{
    ComPtr<IDXGIFactory1> factory;
    if (!CaptureUtil::ok(kTag, "CreateDXGIFactory1",
                         CreateDXGIFactory1(__uuidof(IDXGIFactory1),
                                            reinterpret_cast<void**>(factory.put()))))
        return;

    for (UINT a = 0;; ++a) {
        ComPtr<IDXGIAdapter1> adapter;
        if (factory->EnumAdapters1(a, adapter.put()) == DXGI_ERROR_NOT_FOUND)
            break;
        if (!adapter)
            break;
        for (UINT o = 0;; ++o) {
            ComPtr<IDXGIOutput> output;
            if (adapter->EnumOutputs(o, output.put()) == DXGI_ERROR_NOT_FOUND)
                break;
            if (!output)
                break;
            if (!visit(output.p))
                return;
        }
    }
}

// Ask one encoder MFT whether it accepts an HEVC Main10 output type. Presence of
// an HEVC encoder is not enough: plenty of hardware encodes 8-bit HEVC only.
bool activateAcceptsMain10(IMFActivate* activate)
{
    ComPtr<IMFTransform> mft;
    if (FAILED(activate->ActivateObject(__uuidof(IMFTransform),
                                        reinterpret_cast<void**>(mft.put()))))
        return false;

    ComPtr<IMFMediaType> type;
    bool accepted = false;
    if (SUCCEEDED(MFCreateMediaType(type.put()))
        && SUCCEEDED(type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video))
        && SUCCEEDED(type->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_HEVC))
        && SUCCEEDED(type->SetUINT32(MF_MT_AVG_BITRATE, 20000000))
        && SUCCEEDED(type->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive))
        // MF_MT_VIDEO_PROFILE is the same GUID as MF_MT_MPEG2_PROFILE; the MinGW
        // headers only define the latter name.
        && SUCCEEDED(type->SetUINT32(MF_MT_MPEG2_PROFILE, eAVEncH265VProfile_Main_420_10))
        && SUCCEEDED(MFSetAttributeSize(type.p, MF_MT_FRAME_SIZE, 1920, 1080))
        && SUCCEEDED(MFSetAttributeRatio(type.p, MF_MT_FRAME_RATE, 30, 1))
        && SUCCEEDED(MFSetAttributeRatio(type.p, MF_MT_PIXEL_ASPECT_RATIO, 1, 1))) {
        accepted = SUCCEEDED(mft->SetOutputType(0, type.p, 0));
    }

    activate->ShutdownObject();
    return accepted;
}

} // namespace

QString HdrOutputInfo::describe() const
{
    if (!valid)
        return QStringLiteral("%1: Advanced Color state unavailable")
            .arg(deviceName.isEmpty() ? QStringLiteral("(unknown output)") : deviceName);

    return QStringLiteral("%1 %2x%3: HDR %4, %5-bit, luminance %6–%7 nits (full-frame %8)")
        .arg(deviceName)
        .arg(desktopRect.width())
        .arg(desktopRect.height())
        .arg(hdrActive ? QStringLiteral("Active") : QStringLiteral("Inactive"))
        .arg(bitsPerColor)
        .arg(minLuminanceNits, 0, 'f', 3)
        .arg(maxLuminanceNits, 0, 'f', 0)
        .arg(maxFullFrameLuminanceNits, 0, 'f', 0);
}

QStringList HdrReport::summaryLines() const
{
    QStringList lines;
    lines << QStringLiteral("Windows HDR: %1")
                 .arg(anyHdrActive ? QStringLiteral("Active on at least one display")
                                   : QStringLiteral("Inactive"));
    for (const HdrOutputInfo& output : outputs)
        lines << QStringLiteral("  ") + output.describe();
    if (outputs.isEmpty())
        lines << QStringLiteral("  No DXGI outputs enumerated");
    lines << QStringLiteral("Capture format: %1").arg(captureFormat);
    lines << QStringLiteral("HDR screenshot support: %1").arg(screenshotSupport);
    lines << QStringLiteral("HDR video encoder: %1").arg(videoEncoder);
    lines << QStringLiteral("Active fallback: %1").arg(activeFallback);
    return lines;
}

namespace HdrCapabilities {

HdrOutputInfo forMonitor(HMONITOR monitor)
{
    HdrOutputInfo found;
    if (!monitor)
        return found;

    forEachOutput([&](IDXGIOutput* output) {
        DXGI_OUTPUT_DESC desc{};
        if (FAILED(output->GetDesc(&desc)) || desc.Monitor != monitor)
            return true;
        found = describeOutput(output);
        return false;
    });
    return found;
}

HdrOutputInfo forWindow(HWND hwnd)
{
    if (!hwnd)
        return {};
    return forMonitor(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST));
}

bool hevcMain10Supported()
{
    // MFStartup/MFShutdown are reference counted per process, so pairing them
    // here is safe even while the replay recorder holds its own reference.
    if (!CaptureUtil::ok(kTag, "MFStartup", MFStartup(MF_VERSION, MFSTARTUP_LITE)))
        return false;

    MFT_REGISTER_TYPE_INFO outputType{ MFMediaType_Video, MFVideoFormat_HEVC };
    IMFActivate** activates = nullptr;
    UINT32 count = 0;
    const HRESULT hr = MFTEnumEx(MFT_CATEGORY_VIDEO_ENCODER,
                                 MFT_ENUM_FLAG_ALL | MFT_ENUM_FLAG_SORTANDFILTER,
                                 nullptr, &outputType, &activates, &count);

    bool supported = false;
    if (SUCCEEDED(hr) && activates) {
        for (UINT32 i = 0; i < count; ++i) {
            if (!supported && activateAcceptsMain10(activates[i]))
                supported = true;
            activates[i]->Release();
        }
        CoTaskMemFree(activates);
    }

    MFShutdown();
    return supported;
}

HdrReport query()
{
    HdrReport report;
    forEachOutput([&](IDXGIOutput* output) {
        report.outputs.push_back(describeOutput(output));
        return true;
    });
    for (const HdrOutputInfo& output : report.outputs)
        report.anyHdrActive = report.anyHdrActive || output.hdrActive;

    report.hevcMain10Encoder = hevcMain10Supported();

    // These describe what the pipeline does TODAY. Phase 6's later items change
    // the strings together with the code they describe — never ahead of it.
    report.captureFormat = QStringLiteral("BGRA8 (SDR, 8-bit)");
    report.screenshotSupport = QStringLiteral("Unavailable — screenshots are 8-bit PNG/JPEG");
    report.videoEncoder = report.hevcMain10Encoder
        ? QStringLiteral("HEVC Main10 encoder present (unused — clips are H.264 8-bit)")
        : QStringLiteral("Unavailable — no HEVC Main10 encoder");
    report.activeFallback = report.anyHdrActive
        ? QStringLiteral("SDR H.264 — HDR content is captured without tone mapping")
        : QStringLiteral("SDR H.264 — display is SDR, no fallback needed");
    return report;
}

void logReport(const HdrReport& report)
{
    for (const QString& line : report.summaryLines())
        qInfo().noquote() << QStringLiteral("Hdr: ") + line;
}

} // namespace HdrCapabilities
} // namespace capture
