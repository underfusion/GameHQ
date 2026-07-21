#include "core/ApplicationMutex.h"

ApplicationMutex::ApplicationMutex()
{
    SetLastError(ERROR_SUCCESS);
    m_handle = CreateMutexW(nullptr, FALSE, Name);
    m_firstOwner = m_handle != nullptr && GetLastError() != ERROR_ALREADY_EXISTS;
}

ApplicationMutex::~ApplicationMutex()
{
    if (m_handle)
        CloseHandle(m_handle);
}

bool ApplicationMutex::isHeld()
{
    HANDLE handle = OpenMutexW(SYNCHRONIZE, FALSE, Name);
    if (!handle)
        return false;
    CloseHandle(handle);
    return true;
}
