# Controller Input

> Milestone 0.3. DualSense-first, but everything is rebindable and keyboard always works.

## Stack

Raw Input / HID for Sony pads (vendor 0x054C): DualSense `0x0CE6`, DualSense Edge `0x0DF2`, virtual DualSense `0x0ECC` (DSX DualSense-emulation mode), DS4 v1 `0x05C4`, DS4 v2 `0x09CC`, including DSX/ViGEm virtual DS4. GameHQ also accepts the DS4-compatible virtual HID IDs observed from DSX/ViGEm on the dev machine: `VID_11FF&PID_0847` and `VID_3670&PID_0902`. **USB and Bluetooth report layouts differ** (DualSense enhanced BT: report id 0x31, 2-byte prefix + CRC32; DS4 BT: report id 0x11; short/simple reports use DS4-style button offsets). XInput covers DSX Xbox mode and real Xbox pads; Back/View maps to GameHQ's Share action (standard XInput has no Create/Share button) and the Guide button maps to PS via `XInputGetStateEx` (xinput1_4 ordinal 100) when available. WinMM (`joyGetPosEx`) is the last-resort fallback for virtual/DirectInput controllers that do not appear through Raw Input or XInput; it picks the button mapping from the joystick's vendor/product id (`joyGetDevCaps`) — Sony pads use the DirectInput Sony order (Square/Cross/Circle/Triangle = 0–3, Share = 8, Options = 9, PS = 12), everything else the Xbox order (A/B/X/Y = 0–3, Back = 6, Start = 7).

**HidHide caveat:** DSX installs HidHide to hide the physical pad from other apps. If HidHide is set to hide devices and GameHQ.exe is not on its application whitelist, Raw Input sees nothing even though the pad works in games — add GameHQ.exe in *HidHide Configuration Client → Applications* for native DualSense handling. The WinMM fallback usually still works meanwhile.

### Detection & hot-plug (robustness rules)

- **Per-device state, keyed by Raw Input handle.** Every supported Sony/DS4 pad is tracked with its own layout, button bitmask, stick hysteresis, and last-report timestamp. Handles change on every reconnect, so a fresh handle is simply a new tracked device. This prevents two pads reporting at once (real DualSense + DSX virtual DS4) from corrupting each other's edge detection.
- **One active device per backend, with immediate failover.** The first pad that produces a valid report becomes active; reports from other tracked pads only update their state. If the active pad is removed — or goes silent for >1 s while another tracked pad shows a real input change — the active role fails over immediately instead of waiting for the disconnect debounce. Only when no candidate exists does the 1.5 s debounce run, cancelled by any resumed report.
- **Arrivals probe one device, not the world.** `WM_INPUT_DEVICE_CHANGE` (via `RIDEV_DEVNOTIFY`) carries the device handle; only that handle is queried. A debounced (400 ms) reconciliation pass syncs against `GetRawInputDeviceList` to prune handles Windows no longer lists — removal messages can be lost during heavy re-enumeration (DSX re-creating its virtual pad).
- **Xbox-type pads belong to XInput only.** HID collections whose interface path contains `IG_` are XInput devices; Raw Input ignores them so one physical pad can never drive two backends.
- **Empty XInput slots are never hot-polled** (documented pitfall: `XInputGetState` on an empty slot can stall for milliseconds). Connected slots poll at 33 ms; empty slots are probed only on the Raw Input backend's debounced device-topology hint plus a 3 s safety-net timer. WinMM works the same way: 50 ms polling while connected, arrival scans only on topology hints plus a 2 s safety net.
- **Exactly one backend routes input.** `InputEngine` picks the active backend (Sony HID > XInput > WinMM among the connected ones); events from the others are dropped, since they are usually the same physical pad seen through another API. Switching or losing the active backend cancels held navigation repeat and Share tap/hold state, and XInput/WinMM synthesize release events for held buttons before reporting a disconnect — no stuck buttons or accidental screenshot/replay triggers across transitions.
- **Settings shows live status**: `input.controllerStatus` (green/grey dot row in the input-test card) reports the active backend independently of the last-input line.
- **The left stick doubles as the D-pad, and the rule lives in one place.** `input/StickNav.h` maps a stick's raw x/y onto `Dpad*` bits for every backend; each one only supplies an `AxisConfig`. Deadzone values stay per-backend on purpose — they are tuned against each pad's raw range and are not interchangeable: DualSense `center 128, deadzone 60, return 30`, XInput `center 0, deadzone 12000`, WinMM `center 32767, deadzone 16000`. What is shared is the structure, and it encodes the three traps: **Y polarity is not universal** (DualSense/WinMM report positive = down, XInput positive = up), the two directions on an axis are **mutually exclusive**, and **hysteresis is opt-in** (`returnZone < deadzone` keeps an axis active until the stick returns well inside center; set `returnZone == deadzone` and the rule collapses to a plain threshold). Only the DualSense backend runs hysteresis today — XInput and WinMM never had it and still don't.

## Tap vs Hold (Share)

```txt
button down  → start timer
released < threshold        → TAP action (screenshot)
held ≥ threshold (def. 2 s) → HOLD action (save replay), mark consumed
release after consumed      → nothing
```

Threshold options: 1.0 / 1.5 / 2.0 / 3.0 s / custom. Implemented in the binding runtime's gesture handling (`BindingRuntime`).

**Frame grab while a clip is focused.** In the Playback scope (a clip focused in the overlay or the desktop lightbox), a **Share tap** is bound to `playback.frame_grab` instead of the global screenshot. It grabs the exact frame currently shown on the video surface — paused or mid-playback — and saves it as a screenshot for the clip's game through the normal screenshot pipeline. Because it is a Playback-scope binding, it overrides the global screenshot only while a clip is focused; everywhere else Share tap stays the global screenshot. Keyboard equivalent: **S** (Playback scope only, so it never collides with the global `Ctrl+Shift+S`). Share **hold** is unbound in Playback scope, so it still falls through to the global save-replay action.

## Default mapping

See [product-spec.md §6](product-spec.md#6-controller-mapping-default). PS button is frequently intercepted (Steam, DS4Windows, Game Bar) → treat as optional; fallback overlay toggle: **Share double-tap** and `Ctrl+Shift+G`.

## Bindings

Built-in defaults are merged with sparse rows from `binding_overrides`. `BindingResolver` applies group-wide or device-fingerprint overrides, and `BindingRuntime` resolves press, tap, hold, and double-tap gestures with playback → overlay/desktop → global context precedence. Controller codes are position-based, so the same assignment follows the physical button position across PlayStation, Xbox, Nintendo, and generic pads. The legacy `bindings` table is retained only for database compatibility.

## Overlay routing

`InputEngine` owns the routing gates. Overlay open ⇒ events go to QML navigation and are *not* forwarded anywhere else. Overlay closed ⇒ global triggers (Share tap/hold, PS/toggle) remain available, while desktop-gallery navigation is allowed only when the main GameHQ window is focused **and** the real Win32 foreground window belongs to the GameHQ process. This second foreground-process check is required because RawInput uses `RIDEV_INPUTSINK`, so pad reports still arrive while a game has focus. GameHQ never blocks the pad for the game itself — isolation relies on the game losing focus (see [overlay.md](overlay.md)).

## Main App Select Mode

In the main app gallery, Select mode uses the standard batch-action mapping: **Cross** toggles the focused capture, **Triangle** selects/deselects all visible captures, **Square** opens the delete confirmation, and **Circle** exits Select mode or cancels the confirmation. The destructive delete is always confirmed through `ConfirmDialog`.

## Keyboard fallback (global hotkeys, `RegisterHotKey`)

`Ctrl+Shift+S` screenshot · `Ctrl+Shift+E` save replay · `Ctrl+Shift+G` overlay toggle. The replay buffer itself has no hotkey — it auto-arms while a game is focused (`replay.auto`, Settings → Replay).
