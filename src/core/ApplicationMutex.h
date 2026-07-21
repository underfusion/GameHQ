#pragma once

#include <windows.h>

// Process-lifetime signal used by Setup and Uninstall. QLockFile remains the
// single-instance authority; this mutex only prevents program-file replacement
// while the accepted GameHQ process is alive.
class ApplicationMutex
{
public:
    static constexpr wchar_t Name[] = L"Local\\GameHQApplicationActive";

    ApplicationMutex();
    ~ApplicationMutex();

    ApplicationMutex(const ApplicationMutex &) = delete;
    ApplicationMutex &operator=(const ApplicationMutex &) = delete;

    bool acquired() const { return m_handle != nullptr && m_firstOwner; }
    static bool isHeld();

private:
    HANDLE m_handle = nullptr;
    bool m_firstOwner = false;
};
