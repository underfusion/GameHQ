# Getting started

GameHQ can take screenshots, save the rolling replay buffer, and open its overlay from keyboard, mouse, or controller bindings. Configure these under **Settings → Input**.

## Essential actions

- **Screenshot:** use the screenshot binding or true System Share when the controller reports it.
- **Save replay:** enable the replay buffer, then use the replay binding. The default controller gesture is Hold.
- **Overlay:** use the overlay binding. Guide/Home availability depends on the provider and other controller software.

Bindings support Press, Tap, Hold, exact double/triple tap, and ordered two-button combinations. A single tap fires on release immediately unless a higher tap count on the same control requires the configured multi-tap wait. A completed hold consumes its tap.

## Modern controller support

**Auto** loads the bundled GameInput runtime, uses capability-aware routing, and falls back to Sony Raw Input, XInput, or WinMM if modern input fails. **Off** disables GameInput for troubleshooting. View/Back is a legacy control and remains independently bindable from true Share/Capture.

Use the three-second Controller Probe when a dedicated button is not listed. It observes GameInput, bounded Raw HID changes, and keyboard macros such as Print Screen. Copy the anonymous compatibility report when requesting support.

GameHQ does not currently provide universal controller isolation from games. Opening the overlay does not guarantee that a game cannot also see the physical controller.

See [Controller compatibility](controller-compatibility.md) for reconnect and diagnostic details.
