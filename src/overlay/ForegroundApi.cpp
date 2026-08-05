#include "overlay/ForegroundApi.h"

#include <windows.h>

namespace
{
// Qt's requestActivate() ends in SetForegroundWindow, which Win32 routinely
// rejects with a foreground-lock denial when our thread isn't the active
// input one (e.g. the game owns focus). AttachThreadInput briefly marries our
// input queue to the current foreground's and the target's, which makes
// SetForegroundWindow behave as if the user clicked the target — bypassing
// the lock. Symmetric: used both when stealing focus for the overlay (show)
// and when handing focus back to the game (hide).
class Win32ForegroundApi final : public ForegroundApi
{
public:
    void* foregroundWindow() override
    {
        return GetForegroundWindow();
    }

    bool forceForeground(void* targetV) override
    {
        HWND target = static_cast<HWND>(targetV);
        if (!target)
            return false;
        if (GetForegroundWindow() == target)
            return true;

        const DWORD myThread  = GetCurrentThreadId();
        const HWND  currentFg = GetForegroundWindow();
        const DWORD fgThread  = currentFg ? GetWindowThreadProcessId(currentFg, nullptr) : 0;
        const DWORD tgtThread = GetWindowThreadProcessId(target, nullptr);

        const bool a1 = (fgThread && fgThread != myThread)
                        ? AttachThreadInput(myThread, fgThread, TRUE) : false;
        const bool a2 = (tgtThread != myThread && tgtThread != fgThread)
                        ? AttachThreadInput(myThread, tgtThread, TRUE) : false;

        const bool ok = SetForegroundWindow(target) != 0;
        BringWindowToTop(target);
        SetWindowPos(target, HWND_TOP, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

        if (a2) AttachThreadInput(myThread, tgtThread, FALSE);
        if (a1) AttachThreadInput(myThread, fgThread, FALSE);

        return ok;
    }
};
} // namespace

ForegroundApi* ForegroundApi::createSystem()
{
    return new Win32ForegroundApi;
}
