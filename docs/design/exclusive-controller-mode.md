# Exclusive Controller Mode — design

Status: **design only. Nothing here is implemented, and the build/no-build
decision at the end is "no build" for now.**

This document exists so the idea stops being re-litigated from memory every time
a user reports that the overlay does not block game input. It records the
architecture, the dependency problem that actually blocks it, and the recovery
requirements any implementation would have to meet.

Scope note: this is plain documentation. It is deliberately not a canonical plan
document and not under `docs/plans/` — the project has one canonical plan at a
time, and a second one here would compete with it.

## The problem

GameHQ reads the controller, but so does the game. Both see every button press.
When the overlay is open and the user presses D-pad Left to move through their
captures, the game behind the overlay also receives D-pad Left and moves the
character. GameHQ has no way to stop that: it is a passive reader.

We will not fix this by injecting into game processes. That is a standing
project rule, it breaks anti-cheat, and it is the single fastest way to get the
application classified as malware.

The only approach that works without injection is to stop the game from seeing
the physical device at all.

## Architecture

Three parts, in this order:

1. **Hide the physical pad.** The real controller is cloaked from every process
   except GameHQ, so games cannot enumerate it. On Windows this means a
   filter driver — HidHide is the known implementation.
2. **Present a virtual pad.** GameHQ creates a virtual controller that games
   see instead, and forwards the physical device's state to it. To the game
   nothing has changed.
3. **Neutralize while the overlay is up.** When the overlay opens, GameHQ stops
   forwarding and holds the virtual pad at its neutral state — sticks centered,
   no buttons, triggers at rest. The game sees a controller that is simply not
   being touched. When the overlay closes, forwarding resumes.

Step 3 is the entire point. Steps 1 and 2 exist only to make step 3 possible.

### What must pass through

Forwarding is not one-directional. A design that only pushes input toward the
game breaks things users will notice immediately:

- **Rumble / force feedback** must travel back from the game to the physical
  pad, or every game silently loses haptics.
- **Adaptive triggers and LED state** (DualSense) are the same problem. If they
  cannot be passed through, the mode must say so up front rather than quietly
  degrading a DualSense to a generic pad.
- **Battery and connection state** must reflect the physical device, not the
  virtual one.

Any candidate virtual-pad backend that cannot carry rumble back is not a
candidate.

## The dependency problem

This is the part that blocks the feature, not the architecture.

**ViGEmBus — the obvious choice — is retired.** The upstream project is
archived and unmaintained. Shipping a retired kernel driver as a hard dependency
of a user-facing feature means shipping something that will eventually break on
a Windows update with nobody upstream to fix it.

That leaves three real options, none of them cheap:

| Option | Cost | Risk |
|---|---|---|
| Depend on a maintained community fork of ViGEmBus | Low engineering cost | Fork health is unproven; we inherit whatever its maintenance story turns out to be. Requires an actual evaluation of commit activity and signing status, not a link. |
| Build and sign our own VHF-based kernel driver (WDK) | High. Kernel development, WHQL/EV signing costs, an admin-elevated installer, and a support burden on hardware we do not own | We become responsible for a kernel driver. A bug is a bugcheck on a user's machine, not a crash dialog. |
| Ship nothing; document the limitation | Zero | Users keep hitting the original complaint |

There is no fourth option that avoids a kernel-mode component. Virtual HID
devices cannot be created from user mode on Windows.

**Recommendation: do not build this yet.** The honest reason is the dependency,
not the design. Revisit if a maintained fork demonstrates a real maintenance
track record, or if the reported impact grows enough to justify owning a signed
driver.

## Recovery and safety requirements

If this is ever built, these are not optional. A cloaking feature that fails
open leaves the user with no working controller and no obvious cause.

- **Crash-safe un-hiding.** If GameHQ dies — crash, kill, power loss — the
  physical pad must come back. That means a watchdog or an uninstall path that
  restores visibility without GameHQ running, and a startup check that clears
  any cloak left over from a previous session.
- **Emergency keyboard shortcut.** A global keyboard combination that disables
  the mode immediately. It must be keyboard-only: if the controller is the thing
  that broke, a controller-bound escape hatch is useless.
- **Never assume exclusive ownership of HidHide configuration.** This is the
  0.6.3 lesson, learned the hard way: other applications (DSX among them) write
  the same configuration. GameHQ must add and remove only its own entries and
  must never overwrite or clear the whole config. Reading someone else's cloak
  state and "cleaning it up" is how a user's unrelated setup gets destroyed.
- **Opt-in, default OFF, labeled Experimental.** With a plain-language
  explanation of what gets installed and what happens if it goes wrong.

## Anti-cheat compatibility

State this plainly in the UI, not in a footnote:

Hiding a physical device and presenting a virtual one is indistinguishable, from
an anti-cheat's perspective, from several techniques that anti-cheat exists to
stop. Some anti-cheat systems will refuse to run, some will flag the account.
GameHQ does not inject into game processes and does not synthesize input the
user did not perform, but that is not something the user can prove to an
anti-cheat vendor. Any user enabling this mode must be told, before enabling it,
that it may prevent protected games from launching.

## Relationship to the current input work

Exclusive Controller Mode is the complete fix for overlay input bleed. It is not
the only mitigation, and the cheaper ones are worth doing regardless:

- Honest overlay foreground state, so the overlay does not claim to have focus
  it never acquired.
- A clear statement in the UI that the overlay does not block game input, rather
  than letting users discover it during a match.

Those are tracked separately and do not depend on anything in this document.
