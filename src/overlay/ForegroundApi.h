#pragma once

// Injectable seam over the Win32 foreground surface, so the overlay's
// acquisition/retry logic is testable: real foreground behavior cannot be
// forced deterministically on a live desktop (the shell arbitrates focus),
// but a fake can refuse exactly N times.
class ForegroundApi
{
public:
    virtual ~ForegroundApi() = default;

    virtual void* foregroundWindow() = 0;
    // One attempt to move the OS foreground to `target`, using the
    // AttachThreadInput foreground-lock bypass. Returns what
    // SetForegroundWindow reported; the caller re-reads foregroundWindow()
    // for the truth — Windows may report success and still not move focus.
    virtual bool forceForeground(void* target) = 0;

    // Production implementation (Win32). Caller owns the result.
    static ForegroundApi* createSystem();
};
