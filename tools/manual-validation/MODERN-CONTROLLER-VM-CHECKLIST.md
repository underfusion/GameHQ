# Modern Controller clean-environment checklist

Run this checklist on clean Windows 10 and Windows 11 x64 VMs with no
Microsoft GameInput redistributable preinstalled. Record the OS build, package
SHA-256, transport, controller model and the copied compatibility report.

## Setup

1. Verify the Setup SHA-256 against `release-evidence.json`, then install it for
   the current user without installing a separate GameInput redistributable.
2. Confirm `GameInputRedist.dll` exists beside the installed `app/GameHQ.exe`
   and its SHA-256 is
   `8e34fa4bd769798ddf49cc144e9aa97fa909e640d36982574e5c5e82d3f9cf2d`.
3. Start GameHQ, open Settings > Input and confirm Modern controller support
   reports the app-local 3.5.262 runtime.
4. Connect, disconnect and reconnect one controller while holding a harmless
   button. Confirm no action remains held and the same strong identity restores
   its profile.
5. Turn Modern controller support Off, restart, and confirm legacy controller
   input still works and diagnostics report the intentional fallback.
6. Re-enable Auto, uninstall GameHQ and confirm user settings/captures remain.

## Portable

1. Extract the Portable ZIP into a new directory and confirm `portable.flag`
   plus the same pinned app-local runtime are present.
2. Start the root launcher without installing prerequisites. Confirm Settings >
   Input reports the runtime and copied compatibility report contains no serial
   number, username or full PnP path.
3. Rename `app/GameInputRedist.dll`, start again and confirm GameHQ stays usable
   through legacy input with a clear fallback status; restore the filename.
4. Replace the DLL temporarily with a non-DLL text file, repeat the fallback
   check, then restore the original file and hash.
5. Repeat one USB and one available wireless disconnect/reconnect cycle.

## Evidence

Attach the Setup/Portable hashes, screenshots of the runtime and fallback
states, the redacted compatibility report, and any controller-event log lines.
Do not mark a controller model supported solely because packaging passed.
