// Option B feasibility spike for GameHQ Milestone 0.5.
//
// Goal: prove the project-local MinGW toolchain can drive Windows Graphics Capture
// via hand-written ABI shims (no MSVC, no cppwinrt). We:
//   1. RoInitialize the WinRT apartment
//   2. create a D3D11 device
//   3. bridge it to a WinRT IDirect3DDevice (CreateDirect3D11DeviceFromDXGIDevice)
//   4. create a real top-level HWND
//   5. RoGetActivationFactory -> IGraphicsCaptureItemInterop
//   6. interop->CreateForWindow(hwnd) -> live GraphicsCaptureItem
//   7. read item->get_Size() and print it
//
// If steps 3 and 6 succeed at runtime, the WGC frame pump is buildable on MinGW.

#include "wgc_abi_shims.h"

#include <roapi.h>       // RoInitialize, RoGetActivationFactory
#include <winstring.h>   // WindowsCreateString / WindowsDeleteString
#include <d3d11.h>
#include <dxgi.h>
#include <cstdio>

#define CHECK(step, hr) \
    do { HRESULT _hr = (hr); \
        std::printf("[%-34s] hr=0x%08lX %s\n", step, (unsigned long)_hr, \
                    SUCCEEDED(_hr) ? "OK" : "FAIL"); \
        if (FAILED(_hr)) { failures++; } } while (0)

static const wchar_t* kCaptureItemClass = L"Windows.Graphics.Capture.GraphicsCaptureItem";

int main() {
    int failures = 0;

    HRESULT hr = RoInitialize(RO_INIT_MULTITHREADED);
    CHECK("RoInitialize", hr);

    // 2. D3D11 device
    ID3D11Device* d3dDevice = nullptr;
    ID3D11DeviceContext* d3dCtx = nullptr;
    D3D_FEATURE_LEVEL fl;
    hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
                           D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0,
                           D3D11_SDK_VERSION, &d3dDevice, &fl, &d3dCtx);
    CHECK("D3D11CreateDevice", hr);

    // 3. bridge D3D11 device -> WinRT IDirect3DDevice (via d3d11.dll export)
    IInspectable* winrtDevice = nullptr;
    if (d3dDevice) {
        HMODULE d3dll = LoadLibraryW(L"d3d11.dll");
        auto pfn = d3dll ? (PFN_CreateDirect3D11DeviceFromDXGIDevice)
                               GetProcAddress(d3dll, "CreateDirect3D11DeviceFromDXGIDevice")
                         : nullptr;
        std::printf("[%-34s] %s\n", "CreateDirect3D11...FromDXGIDevice resolve",
                    pfn ? "found in d3d11.dll" : "NOT FOUND");
        if (pfn) {
            IDXGIDevice* dxgiDevice = nullptr;
            hr = d3dDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);
            CHECK("QI IDXGIDevice", hr);
            if (dxgiDevice) {
                hr = pfn(dxgiDevice, &winrtDevice);
                CHECK("D3D->WinRT device bridge", hr);
                dxgiDevice->Release();
            }
        } else {
            failures++;
        }
    }

    // 4. a real, visible top-level window to capture (WGC rejects hidden windows with
    //    E_INVALIDARG). Prefer our own console window if present.
    HWND hwnd = GetConsoleWindow();
    if (!hwnd) {
        hwnd = CreateWindowExW(0, L"STATIC", L"GameHQ WGC spike",
                               WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0, 0, 640, 360,
                               nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);
        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) { DispatchMessageW(&msg); }
    }
    std::printf("[%-34s] hwnd=%p\n", "capture target window", (void*)hwnd);
    if (!hwnd) failures++;

    // 5. activation factory -> IGraphicsCaptureItemInterop
    IGraphicsCaptureItemInterop* interop = nullptr;
    HSTRING classId = nullptr;
    hr = WindowsCreateString(kCaptureItemClass, (UINT32)wcslen(kCaptureItemClass), &classId);
    CHECK("WindowsCreateString(classId)", hr);
    hr = RoGetActivationFactory(classId, IID_IGraphicsCaptureItemInterop, (void**)&interop);
    CHECK("RoGetActivationFactory(interop)", hr);
    if (classId) WindowsDeleteString(classId);

    // 6. HWND -> GraphicsCaptureItem
    IGraphicsCaptureItem* item = nullptr;
    if (interop && hwnd) {
        hr = interop->CreateForWindow(hwnd, IID_IGraphicsCaptureItem, (void**)&item);
        CHECK("interop->CreateForWindow", hr);
    }

    // 7. read the item Size to prove it's live
    if (item) {
        WgcSizeInt32 size{0, 0};
        hr = item->get_Size(&size);
        CHECK("item->get_Size", hr);
        std::printf("[%-34s] %d x %d\n", "captured item size", size.Width, size.Height);
        item->Release();
    }

    if (interop) interop->Release();
    if (winrtDevice) winrtDevice->Release();
    if (d3dCtx) d3dCtx->Release();
    if (d3dDevice) d3dDevice->Release();
    if (hwnd && hwnd != GetConsoleWindow()) DestroyWindow(hwnd);

    std::printf("\n==== SPIKE RESULT: %s (%d failure%s) ====\n",
                failures == 0 ? "PASS" : "FAIL", failures, failures == 1 ? "" : "s");
    return failures == 0 ? 0 : 1;
}
