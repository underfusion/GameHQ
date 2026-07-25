// Sleeps for argv[1] milliseconds and exits. The ProcessIdentity test needs a
// process whose lifetime it controls, so it can ask about one that is still
// running, one that has exited, and an id it has watched being freed.
#include <windows.h>

#include <cstdlib>

int wmain(int argc, wchar_t** argv)
{
    const DWORD milliseconds = argc > 1 ? DWORD(_wtoi(argv[1])) : 0;
    Sleep(milliseconds);
    return 0;
}
