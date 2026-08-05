# Modern controller verification matrix

Use one row per transport and mode. `Pending external validation` is a valid
result; never convert it to Supported from synthetic tests alone.

| Model | Transport / mode | Required checks | Current status |
|---|---|---|---|
| DualSense | USB | Share, Guide, standard controls, reconnect, DSX merge | Pending external validation |
| DualSense | Bluetooth | Share, Guide, sleep/wake, reconnect | Pending external validation |
| Xbox controller | USB | View versus Guide, standard dedup, reconnect | Pending external validation |
| Xbox controller | Bluetooth | View versus Guide, sleep/wake | Pending external validation |
| Xbox controller | Wireless Adapter | receiver unplug/replug, held release | Pending external validation |
| GameSir G7 Pro | available dongle/modes/polling rates | dedicated Capture, probe, gestures, reconnect | **Unverified** |
| 8BitDo Ultimate 2 | USB XInput/DInput | dedicated Capture, extras, mode switch | **Unverified** |
| 8BitDo Ultimate 2 | 2.4 GHz XInput/DInput | Capture, sleep, receiver reconnect | **Unverified** |
| Two controllers | mixed models | independent actions and profiles | Pending external validation |
| Two identical models | same transport | distinct logical IDs and profiles | Pending external validation |

For every completed row attach the anonymous compatibility report, firmware,
Windows build, GameHQ package SHA-256 and a pass/fail result for tap, hold,
double/triple tap, combination and reconnect.
