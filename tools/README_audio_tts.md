# Dialogue TTS Generator

Generates `.wav` voice lines for monster and NPC dialogue directly from `src/game/world/World.cpp`.

## Files

- Script: `tools/generate_dialogue_tts.py`
- Requirements: `tools/requirements-audio.txt`
- Output: `assets/audio/voice/`

## Install

```powershell
pip install -r tools/requirements-audio.txt
```

## Preview without generating files

```powershell
python tools/generate_dialogue_tts.py --dry-run
```

## Generate WAV lines

```powershell
python tools/generate_dialogue_tts.py
```

## Optional voice tuning

```powershell
python tools/generate_dialogue_tts.py --voice-npc-female 0 --voice-npc-male 1 --voice-monster 1 --rate-npc-female 172 --rate-npc-male 160 --rate-monster 135

## Optional Windows voice names (PowerShell backend)
## Optional Windows voice names (PowerShell backend)
```powershell
python tools/generate_dialogue_tts.py --voice-name-npc "Microsoft Zira Desktop" --voice-name-monster "Microsoft David Desktop"
python tools/generate_dialogue_tts.py --voice-name-npc-female "Microsoft Zira Desktop" --voice-name-npc-male "Microsoft David Desktop" --voice-name-monster "Microsoft David Desktop"
```

## Notes

- Tries `pyttsx3` first.
- If `pyttsx3` backend fails, falls back to Windows PowerShell `System.Speech` and still outputs `.wav` files.
- Generates separate folders for `npc_female`, `npc_male`, `panic_female`, and `panic_male`.
- Applies stronger emotional SSML profiles for monster and panic lines on the PowerShell backend.
- If a line begins with `[MONSTER]`, that tag is stripped before speech synthesis.
- A manifest is written to `assets/audio/voice/manifest_dialogue_tts.json`.
