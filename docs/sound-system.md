# Sound System

> Subtle console-style feedback — polished PlayStation vibe, not a noisy desktop utility.

## Events

| Event key | Trigger | Character |
|---|---|---|
| `screenshot` | capture taken | short camera-like click |
| `replay_saved` | clip exported | distinct positive chime |
| `overlay_open` / `overlay_close` | overlay toggled | soft whoosh in/out |
| `nav_tick` | gallery navigation | very quiet tick |
| `favorite` | favorite toggled | small positive blip |
| `confirm` | action confirmed | soft confirm |
| `error` | blocked action / buffer off | quiet error tone |

Files in `assets/sounds/`, overridable per event via `sound_settings` table (enabled, volume 0–100, custom file). Sound packs = folders in `gamehq-data/sound-packs/`.

## Settings

Master on/off · per-event on/off · master volume · pack selection · **"Include GameHQ UI sounds in recordings" — default OFF** (MVP records them via loopback anyway; later use process-loopback capture to exclude our own session).

## Implementation

`SoundEngine` on Qt Multimedia (`QSoundEffect` — low latency, WAV, FFmpeg backend). All 8 effects pre-load from embedded resources (`qrc:/sounds/`) at startup; `play(event)` reads `sounds.enabled` / `sounds.volume` from config live. Exposed to QML as `sounds`.

The **default pack is synthesized** by `assets/sounds/generate_sounds.py` (pure stdlib, license-free, regenerable) — subtle sine tones with fast attack and exponential decay. Replace individual WAVs or add packs under `gamehq-data/sound-packs/` later (1.0).
