#include "capture/AudioCapture.h"
#include "capture/wasapi_shims.h"

#include <windows.h>
#include <audioclient.h>
#include <mmdeviceapi.h>
#include <avrt.h>
#include <propidl.h>

#include <QDebug>
#include <algorithm>
#include <cstring>

namespace {

bool ok(const char* what, HRESULT hr)
{
    if (FAILED(hr)) {
        qWarning().nospace() << "AudioCapture: " << what
                             << " failed hr=0x" << Qt::hex << quint32(hr);
    }
    return SUCCEEDED(hr);
}

long long qpc100nsNow()
{
    LARGE_INTEGER qpc{};
    LARGE_INTEGER freq{};
    QueryPerformanceCounter(&qpc);
    QueryPerformanceFrequency(&freq);
    return (qpc.QuadPart * 10000000LL) / freq.QuadPart;
}

class AudioActivateHandler : public IActivateAudioInterfaceCompletionHandler
{
public:
    AudioActivateHandler()
        : m_event(CreateEventW(nullptr, TRUE, FALSE, nullptr))
    {
    }

    ~AudioActivateHandler()
    {
        if (m_client)
            m_client->Release();
        if (m_event)
            CloseHandle(m_event);
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override
    {
        if (!ppv)
            return E_POINTER;
        if (IsEqualGUID(riid, IID_IUnknown)
            || IsEqualGUID(riid, IID_IActivateAudioInterfaceCompletionHandler)) {
            *ppv = static_cast<IActivateAudioInterfaceCompletionHandler*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override
    {
        return ULONG(InterlockedIncrement(&m_refs));
    }

    ULONG STDMETHODCALLTYPE Release() override
    {
        const LONG refs = InterlockedDecrement(&m_refs);
        if (refs == 0)
            delete this;
        return ULONG(refs);
    }

    HRESULT STDMETHODCALLTYPE ActivateCompleted(IActivateAudioInterfaceAsyncOperation* op) override
    {
        IUnknown* unk = nullptr;
        HRESULT activateHr = E_FAIL;
        m_result = op ? op->GetActivateResult(&activateHr, &unk) : E_POINTER;
        if (SUCCEEDED(m_result))
            m_result = activateHr;
        if (SUCCEEDED(m_result) && unk) {
            m_result = unk->QueryInterface(IID_IAudioClient, reinterpret_cast<void**>(&m_client));
        }
        if (unk)
            unk->Release();
        if (m_event)
            SetEvent(m_event);
        return S_OK;
    }

    bool wait(DWORD ms)
    {
        return m_event && WaitForSingleObject(m_event, ms) == WAIT_OBJECT_0;
    }

    HRESULT result() const { return m_result; }

    IAudioClient* takeClient()
    {
        IAudioClient* out = m_client;
        m_client = nullptr;
        return out;
    }

private:
    LONG m_refs = 1;
    HANDLE m_event = nullptr;
    HRESULT m_result = E_FAIL;
    IAudioClient* m_client = nullptr;
};

} // namespace

AudioCapture::AudioCapture(QObject* parent)
    : QObject(parent)
{
}

AudioCapture::~AudioCapture()
{
    stop();
}

bool AudioCapture::start(unsigned long targetPid)
{
    if (m_active)
        return true;

    // NOTE: AvSetMmThreadCharacteristicsW removed — passing nullptr as the
    // task-index pointer is UB and the suspected trigger of the dev.74/76
    // worker-thread crash on every game-arm. MMCSS boosting is not required
    // for shared-mode loopback capture.

    qInfo() << "AudioCapture: start (targetPid=" << targetPid << ")";

    HRESULT hr = CoCreateInstance(CLSID_MMDeviceEnumerator, nullptr, CLSCTX_ALL,
                                  IID_IMMDeviceEnumerator,
                                  reinterpret_cast<void**>(&m_enumerator));
    if (!ok("CoCreateInstance(MMDeviceEnumerator)", hr))
        return false;

    qInfo() << "AudioCapture: enumerator ready";
    m_startQpc100ns = qpc100nsNow();

    // Process-loopback (ActivateAudioInterfaceAsync) is disabled: it is exotic,
    // untested under MinGW, and crashed the worker thread on every game-arm in
    // dev.74. Desktop loopback (below) is the standard, well-supported path.
    // To re-enable process capture later, validate the async activation surface
    // and completion-handler vtable against a Windows SDK build first.
    if (initDesktopLoopback()) {
        m_active = true;
        return true;
    }

    release();
    return false;
}

bool AudioCapture::initDesktopLoopback()
{
    // NOTE: MinGW does not support MSVC __try/__except SEH, so we cannot catch a
    // hardware AV here. Granular step logging (each WASAPI call preceded by a
    // qInfo) lets us identify the crashing call from the log after a crash.
    return initDesktopLoopbackInner();
}

bool AudioCapture::initDesktopLoopbackInner()
{
    qInfo() << "AudioCapture: initDesktopLoopback — GetDefaultAudioEndpoint";
    HRESULT hr = m_enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &m_device);
    if (!ok("GetDefaultAudioEndpoint(eRender)", hr))
        return false;

    qInfo() << "AudioCapture: Activate(IAudioClient)";
    hr = m_device->Activate(IID_IAudioClient, CLSCTX_ALL, nullptr,
                            reinterpret_cast<void**>(&m_client));
    if (!ok("Activate(IAudioClient)", hr))
        return false;

    WAVEFORMATEX* mix = nullptr;
    qInfo() << "AudioCapture: GetMixFormat";
    hr = m_client->GetMixFormat(&mix);
    if (!ok("GetMixFormat", hr))
        return false;
    qInfo().nospace() << "AudioCapture: mix format " << mix->nSamplesPerSec
                      << "Hz " << mix->nChannels << "ch " << mix->wBitsPerSample << "b";

    WAVEFORMATEXTENSIBLE want{};
    want.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    want.Format.nChannels = mix->nChannels;
    want.Format.nSamplesPerSec = mix->nSamplesPerSec;
    want.Format.wBitsPerSample = 32;
    want.Format.nBlockAlign = WORD(want.Format.nChannels * sizeof(float));
    want.Format.nAvgBytesPerSec = want.Format.nSamplesPerSec * want.Format.nBlockAlign;
    want.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
    want.Samples.wValidBitsPerSample = 32;
    want.dwChannelMask = mix->nChannels == 2
        ? (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT)
        : SPEAKER_FRONT_CENTER;
    want.SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
    CoTaskMemFree(mix);

    qInfo() << "AudioCapture: Initialize(LOOPBACK)";
    hr = m_client->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK,
                              10000000LL, 0, &want.Format, nullptr);
    if (!ok("Initialize(LOOPBACK)", hr))
        return false;

    UINT32 bufFrames = 0;
    qInfo() << "AudioCapture: GetBufferSize";
    hr = m_client->GetBufferSize(&bufFrames);
    if (!ok("GetBufferSize", hr))
        return false;

    m_sampleRate = want.Format.nSamplesPerSec;
    m_channels = want.Format.nChannels;
    m_frameSize = want.Format.nBlockAlign;
    m_bufFrames = bufFrames;

    qInfo() << "AudioCapture: GetService(IAudioCaptureClient)";
    hr = m_client->GetService(IID_IAudioCaptureClient,
                              reinterpret_cast<void**>(&m_capture));
    if (!ok("GetService(IAudioCaptureClient)", hr))
        return false;

    qInfo() << "AudioCapture: client->Start()";
    hr = m_client->Start();
    if (!ok("Start", hr))
        return false;

    qInfo().noquote() << QStringLiteral("AudioCapture: desktop loopback %1Hz %2ch buffer=%3 frames")
        .arg(m_sampleRate).arg(m_channels).arg(m_bufFrames);
    return true;
}

bool AudioCapture::initProcessLoopback(unsigned long pid)
{
    qInfo().noquote() << QStringLiteral("AudioCapture: trying process-loopback for PID %1").arg(pid);

    AUDIOCLIENT_ACTIVATION_PARAMS params{};
    params.ActivationType = AUDIOCLIENT_ACTIVATION_TYPE_PROCESS_LOOPBACK;
    params.ProcessLoopbackParams.TargetProcessId = pid;
    params.ProcessLoopbackParams.ProcessLoopbackMode = PROCESS_LOOPBACK_MODE_INCLUDE_TARGET_PROCESS_TREE;

    PROPVARIANT pv{};
    pv.vt = VT_BLOB;
    pv.blob.cbSize = sizeof(params);
    pv.blob.pBlobData = reinterpret_cast<BYTE*>(&params);

    auto* handler = new AudioActivateHandler();
    IActivateAudioInterfaceAsyncOperation* op = nullptr;
    HRESULT hr = ActivateAudioInterfaceAsync(L"VAD\\Process_Loopback", IID_IAudioClient,
                                             &pv, handler, &op);
    if (!ok("ActivateAudioInterfaceAsync(process loopback)", hr)) {
        handler->Release();
        return false;
    }

    if (!handler->wait(3000)) {
        qWarning() << "AudioCapture: process-loopback activation timed out";
        if (op) op->Release();
        handler->Release();
        return false;
    }

    hr = handler->result();
    m_client = handler->takeClient();
    if (op) op->Release();
    handler->Release();
    if (!ok("process-loopback activation result", hr) || !m_client)
        return false;

    WAVEFORMATEX* mix = nullptr;
    hr = m_client->GetMixFormat(&mix);
    if (!ok("GetMixFormat(process)", hr))
        return false;

    WAVEFORMATEXTENSIBLE want{};
    want.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    want.Format.nChannels = mix->nChannels;
    want.Format.nSamplesPerSec = mix->nSamplesPerSec;
    want.Format.wBitsPerSample = 32;
    want.Format.nBlockAlign = WORD(want.Format.nChannels * sizeof(float));
    want.Format.nAvgBytesPerSec = want.Format.nSamplesPerSec * want.Format.nBlockAlign;
    want.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
    want.Samples.wValidBitsPerSample = 32;
    want.dwChannelMask = mix->nChannels == 2
        ? (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT)
        : SPEAKER_FRONT_CENTER;
    want.SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
    CoTaskMemFree(mix);

    hr = m_client->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK,
                              10000000LL, 0, &want.Format, nullptr);
    if (!ok("Initialize(process loopback)", hr))
        return false;

    UINT32 bufFrames = 0;
    hr = m_client->GetBufferSize(&bufFrames);
    if (!ok("GetBufferSize(process)", hr))
        return false;

    m_sampleRate = want.Format.nSamplesPerSec;
    m_channels = want.Format.nChannels;
    m_frameSize = want.Format.nBlockAlign;
    m_bufFrames = bufFrames;

    hr = m_client->GetService(IID_IAudioCaptureClient,
                              reinterpret_cast<void**>(&m_capture));
    if (!ok("GetService(IAudioCaptureClient/process)", hr))
        return false;

    hr = m_client->Start();
    if (!ok("Start(process)", hr))
        return false;

    qInfo().noquote() << QStringLiteral("AudioCapture: process loopback PID %1 %2Hz %3ch buffer=%4 frames")
        .arg(pid).arg(m_sampleRate).arg(m_channels).arg(m_bufFrames);
    return true;
}

void AudioCapture::stop()
{
    m_active = false;
    release();
}

void AudioCapture::release()
{
    if (m_client && m_capture)
        m_client->Stop();
    if (m_capture) { m_capture->Release(); m_capture = nullptr; }
    if (m_client) { m_client->Release(); m_client = nullptr; }
    if (m_device) { m_device->Release(); m_device = nullptr; }
    if (m_enumerator) { m_enumerator->Release(); m_enumerator = nullptr; }
    m_sampleRate = 0;
    m_channels = 0;
    m_frameSize = 0;
    m_bufFrames = 0;
    m_startQpc100ns = 0;
}

unsigned AudioCapture::poll(float* outBuf, unsigned maxFrames, long long* outTimestamp100ns)
{
    if (outTimestamp100ns)
        *outTimestamp100ns = 0;
    if (!m_active || !m_capture || !outBuf || maxFrames == 0)
        return 0;

    UINT32 packetFrames = 0;
    HRESULT hr = m_capture->GetNextPacketSize(&packetFrames);
    if (FAILED(hr) || packetFrames == 0) {
        if (FAILED(hr))
            qWarning().nospace() << "AudioCapture: GetNextPacketSize hr=0x" << Qt::hex << quint32(hr);
        return 0;
    }

    BYTE* data = nullptr;
    DWORD flags = 0;
    hr = m_capture->GetBuffer(&data, &packetFrames, &flags, nullptr, nullptr);
    if (FAILED(hr) || packetFrames == 0) {
        if (FAILED(hr) && hr != AUDCLNT_S_BUFFER_EMPTY)
            qWarning().nospace() << "AudioCapture: GetBuffer hr=0x" << Qt::hex << quint32(hr);
        return 0;
    }

    const unsigned copied = std::min<unsigned>(packetFrames, maxFrames);
    const size_t bytes = size_t(copied) * m_frameSize;
    if (flags & AUDCLNT_BUFFERFLAGS_SILENT)
        memset(outBuf, 0, bytes);
    else
        memcpy(outBuf, data, bytes);

    if (outTimestamp100ns)
        *outTimestamp100ns = qpc100nsNow() - m_startQpc100ns;

    hr = m_capture->ReleaseBuffer(packetFrames);
    if (FAILED(hr))
        qWarning().nospace() << "AudioCapture: ReleaseBuffer hr=0x" << Qt::hex << quint32(hr);

    return copied;
}
