# Audio Setup (OpenAL)

This project now routes puzzle sound cues through OpenAL.

## Folder layout

- `assets/audio/sfx/puzzle_tick.wav`
- `assets/audio/sfx/puzzle_complete.wav`
- `assets/audio/sfx/puzzle_fail.wav`

You can add more sounds later and map them in `World::Initialize()`.

## Recommended format

- File type: `.wav` (PCM 16-bit)
- Channels: mono for UI/puzzle SFX (stereo is also supported)
- Sample rate: 44100 Hz or 48000 Hz
- Peak level target: around -6 dBFS to avoid clipping

## Current cue mapping

- `puzzle_tick` -> `assets/audio/sfx/puzzle_tick.wav`
- `puzzle_complete` -> `assets/audio/sfx/puzzle_complete.wav`
- `puzzle_fail` -> `assets/audio/sfx/puzzle_fail.wav`
