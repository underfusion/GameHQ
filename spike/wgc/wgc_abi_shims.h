// Option B feasibility spike — hand-written ABI shims for the Windows.Graphics.Capture
// interfaces that mingw-w64 does NOT ship (it only has IGraphicsCaptureSession).
//
// These are the minimal COM/WinRT ABI declarations needed to obtain a
// GraphicsCaptureItem from an HWND and read its Size. If this compiles, links and
// runs under the project-local MinGW toolchain, the WGC frame pump for 0.5 can be
// built without switching to MSVC.
//
// IIDs and vtable layouts are the stable public ABI (Windows SDK / MS docs).
#pragma once

#include <windows.h>
#include <unknwn.h>
#include <inspectable.h>  // IInspectable, TrustLevel (shipped by mingw-w64)
#include <dxgi.h>         // IDXGIDevice (referenced by the D3D interop typedef below)

// Windows.Graphics.SizeInt32 — matches ABI layout { INT32 Width; INT32 Height; }
struct WgcSizeInt32 {
    INT32 Width;
    INT32 Height;
};

// ---- IGraphicsCaptureItem : IInspectable  {79C3F95B-31F7-4EC2-A464-632EF5D30760}
// Only the vtable slots up to get_Size are needed; later slots are stubbed so the
// vtable offset of get_Size is correct.
struct __declspec(uuid("79C3F95B-31F7-4EC2-A464-632EF5D30760"))
IGraphicsCaptureItem : public IInspectable {
    virtual HRESULT STDMETHODCALLTYPE get_DisplayName(HSTRING* value) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_Size(WgcSizeInt32* value) = 0;
    // add_Closed / remove_Closed follow here in the real ABI (not needed for the spike)
};

// ---- IGraphicsCaptureItemInterop : IUnknown  {3628E81B-3CAC-4C60-B7F4-23CE0E0C3356}
// The Win32<->WinRT bridge that turns an HWND/HMONITOR into a GraphicsCaptureItem.
struct __declspec(uuid("3628E81B-3CAC-4C60-B7F4-23CE0E0C3356"))
IGraphicsCaptureItemInterop : public IUnknown {
    virtual HRESULT STDMETHODCALLTYPE CreateForWindow(HWND window, REFIID riid, void** result) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateForMonitor(HMONITOR monitor, REFIID riid, void** result) = 0;
};

// ---- IDirect3DDxgiInterfaceAccess : IUnknown {A9B3D012-3DF2-4EE3-B8D1-8695F457D3C1}
// Bridges a WinRT IDirect3DSurface/IDirect3DDevice back to a raw DXGI/D3D11 interface.
struct __declspec(uuid("A9B3D012-3DF2-4EE3-B8D1-8695F457D3C1"))
IDirect3DDxgiInterfaceAccess : public IUnknown {
    virtual HRESULT STDMETHODCALLTYPE GetInterface(REFIID iid, void** object) = 0;
};

// Manual IIDs (safer/clearer than relying on __uuidof under MinGW).
static const GUID IID_IGraphicsCaptureItem =
    { 0x79C3F95B, 0x31F7, 0x4EC2, { 0xA4, 0x64, 0x63, 0x2E, 0xF5, 0xD3, 0x07, 0x60 } };
static const GUID IID_IGraphicsCaptureItemInterop =
    { 0x3628E81B, 0x3CAC, 0x4C60, { 0xB7, 0xF4, 0x23, 0xCE, 0x0E, 0x0C, 0x33, 0x56 } };

// CreateDirect3D11DeviceFromDXGIDevice — exported from d3d11.dll. Resolved at runtime
// via GetProcAddress so a missing import-lib symbol can't block the link.
typedef HRESULT (WINAPI *PFN_CreateDirect3D11DeviceFromDXGIDevice)(
    IDXGIDevice* dxgiDevice, IInspectable** graphicsDevice);
