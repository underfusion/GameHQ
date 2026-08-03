#include "input/RawInputApi.h"

#include "input/HidCloakMonitor.h"

#include <QByteArray>
#include <QVarLengthArray>

#include <windows.h>

RawInputApi::~RawInputApi() = default;

namespace
{
// The real Win32 Raw Input surface. Nothing here decides anything about pads;
// it only translates Win32 into the platform-clean structs in RawInputApi.h.
class Win32RawInputApi final : public RawInputApi
{
public:
    bool readHeader(void* rawInputHandle, Header& out) override
    {
        // RID_HEADER copies a fixed 24-byte struct into our stack frame: no
        // size probe, no buffer, no allocation — the whole point of reading
        // the header before deciding whether the payload is worth fetching.
        RAWINPUTHEADER header{};
        UINT size = sizeof(header);
        if (GetRawInputData(static_cast<HRAWINPUT>(rawInputHandle), RID_HEADER,
                            &header, &size, sizeof(RAWINPUTHEADER))
            == static_cast<UINT>(-1))
            return false;

        out.device = header.hDevice;
        out.type = header.dwType == RIM_TYPEHID ? DeviceType::Hid : DeviceType::Other;
        return true;
    }

    bool readPayload(void* rawInputHandle, Payload& out) override
    {
        auto handle = static_cast<HRAWINPUT>(rawInputHandle);
        UINT size = 0;
        if (GetRawInputData(handle, RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER)) != 0
            || size == 0)
            return false;

        // One buffer for the life of the backend: it grows to the largest
        // report seen and is then reused, so a pad streaming at 250 Hz (or
        // 8000) does not allocate per event. QByteArray's storage comes from
        // malloc, so it is aligned well enough for RAWINPUT.
        if (m_buffer.size() < static_cast<qsizetype>(size))
            m_buffer.resize(static_cast<qsizetype>(size));
        if (GetRawInputData(handle, RID_INPUT, m_buffer.data(), &size, sizeof(RAWINPUTHEADER))
            != size)
            return false;

        auto* ri = reinterpret_cast<RAWINPUT*>(m_buffer.data());
        if (ri->header.dwType != RIM_TYPEHID)
            return false;

        out.reports     = ri->data.hid.bRawData;
        out.reportCount = static_cast<int>(ri->data.hid.dwCount);
        out.reportSize  = static_cast<int>(ri->data.hid.dwSizeHid);
        return true;
    }

    DeviceInfo describeDevice(void* deviceHandle) override
    {
        DeviceInfo out;
        RID_DEVICE_INFO info{};
        info.cbSize = sizeof(info);
        UINT infoSize = sizeof(info);
        if (GetRawInputDeviceInfoW(deviceHandle, RIDI_DEVICEINFO, &info, &infoSize)
            == static_cast<UINT>(-1))
            return out;   // queried stays false: transient, not a verdict

        out.queried = true;
        out.isHid   = info.dwType == RIM_TYPEHID;
        if (out.isHid) {
            out.vendorId  = info.hid.dwVendorId;
            out.productId = info.hid.dwProductId;
            out.usagePage = info.hid.usUsagePage;
            out.usage     = info.hid.usUsage;
        }
        return out;
    }

    DevicePath devicePath(void* deviceHandle) override
    {
        DevicePath out;
        UINT chars = 0;
        if (GetRawInputDeviceInfoW(deviceHandle, RIDI_DEVICENAME, nullptr, &chars) != 0
            || chars == 0)
            return out;

        QVarLengthArray<wchar_t, 256> buf(static_cast<int>(chars) + 1);
        if (GetRawInputDeviceInfoW(deviceHandle, RIDI_DEVICENAME, buf.data(), &chars)
            == static_cast<UINT>(-1))
            return out;

        buf[buf.size() - 1] = 0;
        out.queried = true;
        out.value   = QString::fromWCharArray(buf.data());
        return out;
    }

    QList<EnumeratedDevice> enumerateDevices() override
    {
        QList<EnumeratedDevice> devices;
        UINT count = 0;
        if (GetRawInputDeviceList(nullptr, &count, sizeof(RAWINPUTDEVICELIST)) != 0 || count == 0)
            return devices;

        QByteArray bytes(static_cast<qsizetype>(count * sizeof(RAWINPUTDEVICELIST)),
                         Qt::Uninitialized);
        auto* list = reinterpret_cast<RAWINPUTDEVICELIST*>(bytes.data());
        if (GetRawInputDeviceList(list, &count, sizeof(RAWINPUTDEVICELIST))
            == static_cast<UINT>(-1))
            return devices;

        devices.reserve(static_cast<qsizetype>(count));
        for (UINT i = 0; i < count; ++i)
            devices.append({ list[i].hDevice,
                             list[i].dwType == RIM_TYPEHID ? DeviceType::Hid : DeviceType::Other });
        return devices;
    }

    CloakScan scanHiddenPads(const QSet<QString>& visibleRawPathsLower) override
    {
        const HidCloakMonitor::ScanResult result = HidCloakMonitor::scan(visibleRawPathsLower);
        return CloakScan{ result.hiddenPads, result.hidHidePresent };
    }

private:
    QByteArray m_buffer;
};
} // namespace

RawInputApi* RawInputApi::createSystem()
{
    return new Win32RawInputApi;
}
