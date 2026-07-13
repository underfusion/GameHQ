#pragma once

// 0.5 Step 7 — WASAPI loopback audio capture.
//
// Captures system audio (or per-process when PID is provided) from the default
// render device in shared loopback mode. Runs on the same MTA worker thread as
// the WGC frame pump; all COM/WASAPI calls are on that thread.
//
// Flow:
//   start() → IMMDeviceEnumerator → GetDefaultAudioEndpoint → Activate(IAudioClient)
//          → Initialize(AUDCLNT_STREAMFLAGS_LOOPBACK) → GetMixFormat
//          → GetService(IAudioCaptureClient) → Start
//   poll() → IAudioCaptureClient::GetBuffer → AAC encoder → sample queue
//   stop()  → Stop + release
//
// Process-loopback (Win10 21H2+): if a target PID is provided, tries
// ActivateAudioInterfaceAsync with CLSID_AudioClientProcessLoopback.
// Falls back to desktop loopback on failure.

#include <QObject>
#include <QString>

#ifndef WAVE_FORMAT_PCM
#define WAVE_FORMAT_PCM 1
#endif

// fwd
struct IAudioClient;
struct IAudioCaptureClient;
struct IMMDevice;
struct IMMDeviceEnumerator;

class AudioCapture : public QObject
{
    Q_OBJECT
public:
    explicit AudioCapture(QObject* parent = nullptr);
    ~AudioCapture() override;

    // Start capturing from the default render endpoint.
    // If pid != 0, attempts process-scoped loopback first, falls back to desktop.
    // Returns true if audio capture is active (either mode).
    bool start(unsigned long targetPid = 0);

    // Stop capture and release all WASAPI/COM resources.
    void stop();

    // Poll available audio frames. Copies up to maxFrames worth of interleaved
    // float32 samples into outBuf. Returns the number of frames actually copied.
    // outTimestamp100ns is the QPC timestamp of the first sample in this batch.
    // Safe to call when inactive (returns 0).
    unsigned poll(float* outBuf, unsigned maxFrames, long long* outTimestamp100ns);

    bool isActive() const { return m_active; }
    unsigned sampleRate() const { return m_sampleRate; }
    unsigned channels() const { return m_channels; }
    // QPC epoch (100 ns) of this capture's t=0 — lets the caller re-express
    // poll() timestamps on a shared clock with the video frames.
    long long startQpc100ns() const { return m_startQpc100ns; }

signals:
    void errorOccurred(const QString& message);

private:
    bool initDesktopLoopback();
    bool initDesktopLoopbackInner();
    bool initProcessLoopback(unsigned long pid);
    void release();

    IAudioClient*          m_client   = nullptr;
    IAudioCaptureClient*   m_capture  = nullptr;
    IMMDevice*             m_device   = nullptr;
    IMMDeviceEnumerator*   m_enumerator = nullptr;

    unsigned m_sampleRate = 0;
    unsigned m_channels   = 0;
    unsigned m_frameSize  = 0;       // bytes per frame (sampleRate * channels * sizeof(float))
    unsigned m_bufFrames  = 0;       // buffer size in frames

    // First poll timestamp — used to compute relative offsets
    long long m_startQpc100ns = 0;
    bool      m_active = false;
};
