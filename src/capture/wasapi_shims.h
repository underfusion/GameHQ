#pragma once
// 0.5 Step 7 — WASAPI process-loopback activation shims.
//
// MinGW's WIDL-generated audioclient.h/mmdeviceapi.h lack the process-loopback
// activation structures that shipped in Windows 10 21H2+. We hand-declare the
// stable ABI (GUIDs + structs) the same way wgc_shims.h did for WGC.
//
// ProcessLoopback: captures audio from a specific process by PID, not the whole
// desktop. If unavailable (pre-21H2 or blocked), fall back to desktop loopback.

#include <windows.h>
#include <initguid.h>    // DEFINE_GUID for the activation GUID

// --- CLSID for process-loopback activator (Win10 21H2+) ---
// {C84D3F32-F73B-40AA-A8AE-210A2FA5E35C}
DEFINE_GUID(CLSID_AudioClientProcessLoopback,
    0xc84d3f32, 0xf73b, 0x40aa, 0xa8, 0xae, 0x21, 0x0a, 0x2f, 0xa5, 0xe3, 0x5c);

// --- PROCESS_LOOPBACK_MODE enum ---
enum PROCESS_LOOPBACK_MODE
{
    PROCESS_LOOPBACK_MODE_INCLUDE_TARGET_PROCESS_TREE = 0,
    PROCESS_LOOPBACK_MODE_EXCLUDE_TARGET_PROCESS_TREE = 1
};

// --- AUDIOCLIENT_PROCESS_LOOPBACK_PARAMS ---
// Passed as the activation params when creating a process-loopback audio client.
struct AUDIOCLIENT_PROCESS_LOOPBACK_PARAMS
{
    DWORD                  TargetProcessId;
    PROCESS_LOOPBACK_MODE  ProcessLoopbackMode;
};

// --- AUDIOCLIENT_ACTIVATION_TYPE enum ---
enum AUDIOCLIENT_ACTIVATION_TYPE
{
    AUDIOCLIENT_ACTIVATION_TYPE_DEFAULT           = 0,
    AUDIOCLIENT_ACTIVATION_TYPE_PROCESS_LOOPBACK  = 1
};

// --- AUDIOCLIENT_ACTIVATION_PARAMS ---
// Wrapper passed to ActivateAudioInterfaceAsync for process-loopback.
struct AUDIOCLIENT_ACTIVATION_PARAMS
{
    AUDIOCLIENT_ACTIVATION_TYPE  ActivationType;
    union
    {
        AUDIOCLIENT_PROCESS_LOOPBACK_PARAMS ProcessLoopbackParams;
    };
};
