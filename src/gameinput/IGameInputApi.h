#pragma once

#include "gameinput/GameInputEvent.h"

#include <QString>

#include <functional>

namespace ModernInput {

class IGameInputApi
{
public:
    using CallbackToken = quint64;
    using EventSink = std::function<void(GameInputEvent)>;

    virtual ~IGameInputApi() = default;

    virtual bool initialize(QString& error) = 0;
    virtual CallbackToken registerDeviceCallback(EventSink sink) = 0;
    virtual CallbackToken registerReadingCallback(EventSink sink) = 0;
    virtual CallbackToken registerSystemButtonCallback(EventSink sink) = 0;
    virtual void stopCallback(CallbackToken token) = 0;
    virtual bool unregisterCallback(CallbackToken token) = 0;
    virtual void releaseDevices() = 0;
    virtual void unload() = 0;
    virtual bool loaded() const = 0;
    virtual QString runtimeDescription() const = 0;
};

// Move-only lifetime guard. reset() always performs StopCallback before
// UnregisterCallback, matching GameInput's callback shutdown contract.
class GameInputCallbackRegistration
{
public:
    GameInputCallbackRegistration() = default;
    GameInputCallbackRegistration(IGameInputApi* api, IGameInputApi::CallbackToken token)
        : m_api(api), m_token(token) {}
    ~GameInputCallbackRegistration() { reset(); }

    GameInputCallbackRegistration(const GameInputCallbackRegistration&) = delete;
    GameInputCallbackRegistration& operator=(const GameInputCallbackRegistration&) = delete;

    GameInputCallbackRegistration(GameInputCallbackRegistration&& other) noexcept
        : m_api(other.m_api), m_token(other.m_token)
    {
        other.m_api = nullptr;
        other.m_token = 0;
    }

    GameInputCallbackRegistration& operator=(GameInputCallbackRegistration&& other) noexcept
    {
        if (this != &other) {
            reset();
            m_api = other.m_api;
            m_token = other.m_token;
            other.m_api = nullptr;
            other.m_token = 0;
        }
        return *this;
    }

    bool valid() const { return m_api != nullptr && m_token != 0; }
    IGameInputApi::CallbackToken token() const { return m_token; }

    void reset()
    {
        if (!valid())
            return;
        IGameInputApi* api = m_api;
        const auto token = m_token;
        m_api = nullptr;
        m_token = 0;
        api->stopCallback(token);
        api->unregisterCallback(token);
    }

private:
    IGameInputApi* m_api = nullptr;
    IGameInputApi::CallbackToken m_token = 0;
};

} // namespace ModernInput

