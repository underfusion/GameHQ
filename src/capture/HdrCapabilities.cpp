#include "capture/HdrCapabilities.h"

#include "capture/CaptureUtil.h"
#include "capture/hdr/HdrStateResolver.h"

#include <codecapi.h>
#include <dxgi1_6.h>
#include <mfapi.h>
#include <mferror.h>
#include <mfidl.h>

#include <QDebug>

#include <optional>
#include <vector>

namespace capture {
namespace {

constexpr const char* kTag = "Hdr";

struct DisplayConfigColorState
{
    std::optional<bool> hdrActive;
    int activeColorMode = -1;
    float sdrWhiteLevelNits = 0.0f;
};

// Windows 11's newer query separates SDR, wide-colour (WCG/ACM), and HDR.
// The MinGW 13 SDK predates these declarations, so keep the ABI-compatible
// definitions local until the bundled toolchain provides them.
constexpr DISPLAYCONFIG_DEVICE_INFO_TYPE kGetAdvancedColorInfo2 =
    static_cast<DISPLAYCONFIG_DEVICE_INFO_TYPE>(15);
enum class AdvancedColorMode : UINT32
{
    Sdr = 0,
    Wcg = 1,
    Hdr = 2
};
struct DisplayConfigAdvancedColorInfo2
{
    DISPLAYCONFIG_DEVICE_INFO_HEADER header;
    union {
        struct {
            UINT32 advancedColorSupported : 1;
            UINT32 advancedColorActive : 1;
            UINT32 reserved1 : 1;
            UINT32 advancedColorLimitedByPolicy : 1;
            UINT32 highDynamicRangeSupported : 1;
            UINT32 highDynamicRangeUserEnabled : 1;
            UINT32 wideColorSupported : 1;
            UINT32 wideColorUserEnabled : 1;
            UINT32 reserved : 24;
        };
        UINT32 value;
    };
    DISPLAYCONFIG_COLOR_ENCODING colorEncoding;
    UINT32 bitsPerColorChannel;
    AdvancedColorMode activeColorMode;
};

// Query the active Windows display configuration behind the Settings "Use
// HDR" toggle. Mapping through DISPLAYCONFIG_SOURCE_DEVICE_NAME joins an
// active path to DXGI's \\.\DISPLAYn name even on hybrid-GPU systems.
DisplayConfigColorState displayColorStateForGdiDevice(const QString& deviceName)
{
    for (int attempt = 0; attempt < 3; ++attempt) {
        UINT32 pathCount = 0;
        UINT32 modeCount = 0;
        if (GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &pathCount, &modeCount)
            != ERROR_SUCCESS) {
            return {};
        }

        std::vector<DISPLAYCONFIG_PATH_INFO> paths(pathCount);
        std::vector<DISPLAYCONFIG_MODE_INFO> modes(modeCount);
        const LONG query = QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &pathCount, paths.data(),
                                              &modeCount, modes.data(), nullptr);
        if (query == ERROR_INSUFFICIENT_BUFFER)
            continue;
        if (query != ERROR_SUCCESS)
            return {};
        paths.resize(pathCount);

        for (const DISPLAYCONFIG_PATH_INFO& path : paths) {
            DISPLAYCONFIG_SOURCE_DEVICE_NAME source{};
            source.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
            source.header.size = sizeof(source);
            source.header.adapterId = path.sourceInfo.adapterId;
            source.header.id = path.sourceInfo.id;
            if (DisplayConfigGetDeviceInfo(&source.header) != ERROR_SUCCESS
                || QString::fromWCharArray(source.viewGdiDeviceName)
                       .compare(deviceName, Qt::CaseInsensitive) != 0) {
                continue;
            }

            DisplayConfigColorState state;
            DisplayConfigAdvancedColorInfo2 color2{};
            color2.header.type = kGetAdvancedColorInfo2;
            color2.header.size = sizeof(color2);
            color2.header.adapterId = path.targetInfo.adapterId;
            color2.header.id = path.targetInfo.id;
            if (DisplayConfigGetDeviceInfo(&color2.header) == ERROR_SUCCESS) {
                state.activeColorMode = int(color2.activeColorMode);
                state.hdrActive = color2.activeColorMode == AdvancedColorMode::Hdr;
            } else {
                // Windows 10 and older display drivers expose only a combined
                // Advanced Color switch. On those systems it remains the best
                // available runtime HDR signal.
                DISPLAYCONFIG_GET_ADVANCED_COLOR_INFO legacy{};
                legacy.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_ADVANCED_COLOR_INFO;
                legacy.header.size = sizeof(legacy);
                legacy.header.adapterId = path.targetInfo.adapterId;
                legacy.header.id = path.targetInfo.id;
                if (DisplayConfigGetDeviceInfo(&legacy.header) != ERROR_SUCCESS)
                    return {};
                state.hdrActive = legacy.advancedColorEnabled != 0;
            }

            DISPLAYCONFIG_SDR_WHITE_LEVEL white{};
            white.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SDR_WHITE_LEVEL;
            white.header.size = sizeof(white);
            white.header.adapterId = path.targetInfo.adapterId;
            white.header.id = path.targetInfo.id;
            if (DisplayConfigGetDeviceInfo(&white.header) == ERROR_SUCCESS
                && white.SDRWhiteLevel > 0) {
                // Windows reports a multiplier of the 80-nit scRGB reference
                // white, multiplied by 1000.
                state.sdrWhiteLevelNits =
                    float(white.SDRWhiteLevel) / 1000.0f * 80.0f;
            }
            return state;
        }
        return {};
    }
    return {};
}

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
    const bool dxgiPqActive =
        desc.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;
    const DisplayConfigColorState windowsColor =
        displayColorStateForGdiDevice(QString::fromWCharArray(desc.DeviceName));
    info.hdrActive =
        hdr::resolveHdrActive(dxgiPqActive, windowsColor.hdrActive);
    info.advancedColorMode = windowsColor.activeColorMode;
    info.sdrWhiteLevelNits = windowsColor.sdrWhiteLevelNits;
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

void applyCapturePolicy(HdrReport& report, bool experimentalHdrEnabled)
{
    if (experimentalHdrEnabled) {
        report.captureFormat = QStringLiteral(
            "FP16 scRGB requested for HDR games; tone-mapped BGRA8 SDR output");
        report.screenshotSupport = QStringLiteral(
            "Tone-mapped SDR PNG/JPEG via WGC when the FP16 path arms");
        report.videoEncoder = report.hevcMain10Encoder
            ? QStringLiteral("HEVC Main10 present; SDR replay currently uses tone-mapped H.264")
            : QStringLiteral("SDR replay uses tone-mapped H.264; no HEVC Main10 encoder");
        report.activeFallback = report.anyHdrActive
            ? QStringLiteral("GPU tone-mapped SDR; any FP16 failure is logged explicitly")
            : QStringLiteral("SDR capture — display is SDR, no HDR conversion needed");
    } else {
        report.captureFormat = QStringLiteral("BGRA8 (SDR, 8-bit)");
        report.screenshotSupport = QStringLiteral(
            "Unavailable — screenshots are 8-bit PNG/JPEG");
        report.videoEncoder = report.hevcMain10Encoder
            ? QStringLiteral("HEVC Main10 encoder present (unused — clips are H.264 8-bit)")
            : QStringLiteral("Unavailable — no HEVC Main10 encoder");
        report.activeFallback = report.anyHdrActive
            ? QStringLiteral("SDR H.264 — HDR content is captured without tone mapping")
            : QStringLiteral("SDR H.264 — display is SDR, no fallback needed");
    }
}

HdrReport queryDisplays(bool experimentalHdrEnabled,
                        const std::optional<bool>& knownHevcMain10)
{
    HdrReport report;
    forEachOutput([&](IDXGIOutput* output) {
        report.outputs.push_back(describeOutput(output));
        return true;
    });
    for (const HdrOutputInfo& output : report.outputs)
        report.anyHdrActive = report.anyHdrActive || output.hdrActive;

    report.hevcMain10Encoder = knownHevcMain10.has_value()
        ? *knownHevcMain10
        : HdrCapabilities::hevcMain10Supported();
    applyCapturePolicy(report, experimentalHdrEnabled);
    return report;
}

} // namespace

QString HdrOutputInfo::describe() const
{
    if (!valid)
        return QStringLiteral("%1: Advanced Color state unavailable")
            .arg(deviceName.isEmpty() ? QStringLiteral("(unknown output)") : deviceName);

    const QString colorMode = advancedColorMode == 2
        ? QStringLiteral(", mode HDR")
        : advancedColorMode == 1
            ? QStringLiteral(", mode WCG")
            : advancedColorMode == 0 ? QStringLiteral(", mode SDR") : QString();
    const QString sdrWhite = sdrWhiteLevelNits > 0.0f
        ? QStringLiteral(", SDR white %1 nits").arg(sdrWhiteLevelNits, 0, 'f', 0)
        : QString();
    return QStringLiteral("%1 %2x%3: HDR %4%5, %6-bit, luminance %7–%8 nits (full-frame %9%10)")
        .arg(deviceName)
        .arg(desktopRect.width())
        .arg(desktopRect.height())
        .arg(hdrActive ? QStringLiteral("Active") : QStringLiteral("Inactive"))
        .arg(colorMode)
        .arg(bitsPerColor)
        .arg(minLuminanceNits, 0, 'f', 3)
        .arg(maxLuminanceNits, 0, 'f', 0)
        .arg(maxFullFrameLuminanceNits, 0, 'f', 0)
        .arg(sdrWhite);
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

HdrReport query(bool experimentalHdrEnabled)
{
    return queryDisplays(experimentalHdrEnabled, std::nullopt);
}

HdrReport refreshDisplayState(const HdrReport& previous,
                              bool experimentalHdrEnabled)
{
    return queryDisplays(experimentalHdrEnabled, previous.hevcMain10Encoder);
}

bool sameDisplayState(const HdrReport& left, const HdrReport& right)
{
    if (left.anyHdrActive != right.anyHdrActive
        || left.outputs.size() != right.outputs.size()) {
        return false;
    }

    for (qsizetype index = 0; index < left.outputs.size(); ++index) {
        const HdrOutputInfo& a = left.outputs.at(index);
        const HdrOutputInfo& b = right.outputs.at(index);
        if (a.deviceName != b.deviceName
            || a.desktopRect != b.desktopRect
            || a.valid != b.valid
            || a.hdrActive != b.hdrActive
            || a.advancedColorMode != b.advancedColorMode
            || a.bitsPerColor != b.bitsPerColor
            || a.minLuminanceNits != b.minLuminanceNits
            || a.maxLuminanceNits != b.maxLuminanceNits
            || a.maxFullFrameLuminanceNits != b.maxFullFrameLuminanceNits
            || a.sdrWhiteLevelNits != b.sdrWhiteLevelNits) {
            return false;
        }
    }
    return true;
}

void logReport(const HdrReport& report)
{
    for (const QString& line : report.summaryLines())
        qInfo().noquote() << QStringLiteral("Hdr: ") + line;
}

} // namespace HdrCapabilities
} // namespace capture
