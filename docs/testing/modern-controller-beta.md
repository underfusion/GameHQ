# Modern controller hardware beta

This is an instrumented local beta, not a stable release. Extract the Portable
ZIP into a new folder, keep **Modern controller support** set to **Auto**, and
do not replace an installed GameHQ copy.

## Send before testing

Record the controller model, firmware, connection (USB, Bluetooth or 2.4 GHz),
controller mode, Windows version and any remapper/interceptor software. Verify
the ZIP SHA-256 against the adjacent checksum file.

## Test sequence

1. Open Settings > Input. Confirm the runtime status, providers, Share/Guide
   availability and extra-button count look plausible.
2. Press the dedicated Share/Capture button in the binding editor. It must be
   labelled **System Share**, **Extra Button N**, a raw HID usage, or a keyboard
   macro—never guessed from the controller model.
3. Run the three-second Controller Probe and press the dedicated button once.
4. Assign the control separately as single tap, hold, double tap, triple tap and
   as the first control of a two-button combination. Verify exactly one intended
   action fires in every case.
5. Hold a harmless bound button while disconnecting, sleeping or unplugging the
   receiver. Reconnect and confirm no action remains stuck and the profile is
   restored only for the same strongly identified controller.
6. If available, repeat with two controllers, two identical models, every
   wireless transport and every firmware input mode.
7. Turn Modern controller support **Off**, restart and confirm legacy controls
   still work. Return it to **Auto** afterward.

## Evidence collection

Click **Copy controller compatibility report** and paste it with the matrix row
and observed result. Attach `gamehq.log` only after reproducing a failure. The
report intentionally omits serial numbers, usernames and full PnP paths; review
attachments before sending anyway.

For each failure include the expected action, actual action, whether it happened
on press or release, and whether another action fired. A model remains
**Unverified** until its own hardware result is received.
