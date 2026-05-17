#!/usr/bin/env python3
"""Generate monster and NPC voice WAV files from dialogue text in src/game/world/World.cpp.

Usage examples:
  python tools/generate_dialogue_tts.py --dry-run
  python tools/generate_dialogue_tts.py
  python tools/generate_dialogue_tts.py --voice-npc 0 --voice-monster 1
"""

from __future__ import annotations

import argparse
import html
import json
import re
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Dict, List

ROOT = Path(__file__).resolve().parents[1]
WORLD_CPP = ROOT / "src" / "game" / "world" / "World.cpp"
OUT_ROOT = ROOT / "assets" / "audio" / "voice"


def _extract_cpp_strings(block: str) -> List[str]:
    pattern = re.compile(r'"((?:\\.|[^"\\])*)"')
    values: List[str] = []
    for match in pattern.finditer(block):
        raw = match.group(1)
        decoded = bytes(raw, "utf-8").decode("unicode_escape")
        values.append(decoded)
    return values


def _extract_array_block(source: str, array_name: str) -> str:
    marker = f"{array_name} = {{"
    start = source.find(marker)
    if start == -1:
        return ""
    brace_start = source.find("{", start)
    if brace_start == -1:
        return ""

    depth = 0
    i = brace_start
    while i < len(source):
        ch = source[i]
        if ch == "{":
            depth += 1
        elif ch == "}":
            depth -= 1
            if depth == 0:
                return source[brace_start : i + 1]
        i += 1

    return ""


def _sanitize_text(line: str) -> str:
    text = line.strip()
    if text.startswith("[MONSTER]"):
        text = text.replace("[MONSTER]", "", 1).strip()
    return text


def _slug(text: str, max_len: int = 40) -> str:
    cleaned = re.sub(r"[^a-zA-Z0-9]+", "_", text.lower()).strip("_")
    if not cleaned:
        cleaned = "line"
    return cleaned[:max_len].rstrip("_")


def _collect_dialogue(source: str) -> Dict[str, List[str]]:
    arrays = {
        "npc_general": "generalLines",
        "npc_neighbor": "lines",  # used in GetNpcNeighborLine after this is encountered
        "panic": "lines",  # used in GetPanicLine after this is encountered
        "monster_taunts": "taunts",
        "monster_screams": "screams",
        "reply_boyd": "boydLines",
        "reply_jade": "jadeLines",
        "reply_tabitha": "tabithaLines",
        "reply_victor": "victorLines",
        "reply_sara": "saraLines",
        "monster_responses": "lines",  # used in GetCharacterMonsterResponseLine
    }

    # Use function scopes for ambiguous array names like "lines"
    sections = {
        "GetNpcLine": ("std::string GetNpcLine", "std::string GetCharacterNpcReplyLine"),
        "GetNpcNeighborLine": ("std::string GetNpcNeighborLine", "std::string GetPanicLine"),
        "GetPanicLine": ("std::string GetPanicLine", "void CreateColoredCubeMesh"),
        "GetCharacterMonsterResponseLine": (
            "std::string GetCharacterMonsterResponseLine",
            "std::string GetMonsterScreamLine",
        ),
        "GetMonsterTauntLine": ("std::string GetMonsterTauntLine", "std::string GetCharacterMonsterResponseLine"),
        "GetMonsterScreamLine": ("std::string GetMonsterScreamLine", "void CreateColoredCubeMesh"),
        "GetCharacterNpcReplyLine": ("std::string GetCharacterNpcReplyLine", "std::string GetCharacterPairLine"),
    }

    def section_text(name: str) -> str:
        begin_marker, end_marker = sections[name]
        start = source.find(begin_marker)
        if start == -1:
            return ""
        end = source.find(end_marker, start + len(begin_marker))
        if end == -1:
            end = len(source)
        return source[start:end]

    result: Dict[str, List[str]] = {}

    npc_section = section_text("GetNpcLine")
    result["npc_general"] = _extract_cpp_strings(_extract_array_block(npc_section, arrays["npc_general"]))

    reply_section = section_text("GetCharacterNpcReplyLine")
    for key in ["reply_boyd", "reply_jade", "reply_tabitha", "reply_victor", "reply_sara"]:
        arr = arrays[key]
        result[key] = _extract_cpp_strings(_extract_array_block(reply_section, arr))

    neighbor_section = section_text("GetNpcNeighborLine")
    result["npc_neighbor"] = _extract_cpp_strings(_extract_array_block(neighbor_section, "lines"))

    panic_section = section_text("GetPanicLine")
    result["panic"] = _extract_cpp_strings(_extract_array_block(panic_section, "lines"))

    taunt_section = section_text("GetMonsterTauntLine")
    result["monster_taunts"] = _extract_cpp_strings(_extract_array_block(taunt_section, arrays["monster_taunts"]))

    response_section = section_text("GetCharacterMonsterResponseLine")
    result["monster_responses"] = _extract_cpp_strings(_extract_array_block(response_section, "lines"))

    scream_section = section_text("GetMonsterScreamLine")
    result["monster_screams"] = _extract_cpp_strings(_extract_array_block(scream_section, arrays["monster_screams"]))

    for key, lines in result.items():
        result[key] = [_sanitize_text(line) for line in lines if line.strip()]

    return result


def _ensure_dirs() -> Dict[str, Path]:
    folders = {
        "monster": OUT_ROOT / "monster",
        "npc_female": OUT_ROOT / "npc_female",
        "npc_male": OUT_ROOT / "npc_male",
        "responses": OUT_ROOT / "responses",
        "panic_female": OUT_ROOT / "panic_female",
        "panic_male": OUT_ROOT / "panic_male",
    }
    for p in folders.values():
        p.mkdir(parents=True, exist_ok=True)
    return folders


def _save_manifest(manifest: Dict[str, object]) -> None:
    manifest_path = OUT_ROOT / "manifest_dialogue_tts.json"
    manifest_path.parent.mkdir(parents=True, exist_ok=True)
    manifest_path.write_text(json.dumps(manifest, indent=2), encoding="utf-8")


def _generate_with_pyttsx3(plan: List[Dict[str, str]], args: argparse.Namespace) -> bool:
    try:
        import pyttsx3  # type: ignore
    except Exception:
        return False

    try:
        engine = pyttsx3.init()
        voices = engine.getProperty("voices")
        if not voices:
            return False

        npc_voice_female = voices[min(max(args.voice_npc_female, 0), len(voices) - 1)].id
        npc_voice_male = voices[min(max(args.voice_npc_male, 0), len(voices) - 1)].id
        monster_voice = voices[min(max(args.voice_monster, 0), len(voices) - 1)].id

        for item in plan:
            out_path = Path(item["path"])
            text = item["text"]
            group = item["group"]

            if group.startswith("monster"):
                engine.setProperty("voice", monster_voice)
                engine.setProperty("rate", args.rate_monster)
            elif group.startswith("npc_female") or group.startswith("panic_female"):
                engine.setProperty("voice", npc_voice_female)
                engine.setProperty("rate", args.rate_npc_female)
            elif group.startswith("npc_male") or group.startswith("panic_male"):
                engine.setProperty("voice", npc_voice_male)
                engine.setProperty("rate", args.rate_npc_male)
            else:
                engine.setProperty("voice", npc_voice_male)
                engine.setProperty("rate", args.rate_npc_male)

            out_path.parent.mkdir(parents=True, exist_ok=True)
            engine.save_to_file(text, str(out_path))

        engine.runAndWait()
        print("[TTS] Engine: pyttsx3")
        return True
    except Exception as ex:
        print(f"[TTS] pyttsx3 failed: {ex}")
        return False


def _generate_with_powershell(plan: List[Dict[str, str]], args: argparse.Namespace) -> bool:
    if not sys.platform.startswith("win"):
        return False

    ps_script = (
        "param(\n"
        "  [string]$Text,\n"
        "  [string]$OutPath,\n"
        "  [int]$Rate,\n"
        "  [string]$VoiceName,\n"
        "  [int]$UseSsml\n"
        ")\n"
        "Add-Type -AssemblyName System.Speech\n"
        "$synth = New-Object System.Speech.Synthesis.SpeechSynthesizer\n"
        "if ($VoiceName -ne '') { try { $synth.SelectVoice($VoiceName) } catch {} }\n"
        "$synth.Rate = $Rate\n"
        "$synth.SetOutputToWaveFile($OutPath)\n"
        "$useSsmlFlag = ($UseSsml -ne 0)\n"
        "if ($useSsmlFlag) { $synth.SpeakSsml($Text) } else { $synth.Speak($Text) }\n"
        "$synth.Dispose()\n"
    )

    with tempfile.NamedTemporaryFile("w", suffix=".ps1", delete=False, encoding="utf-8") as ps_file:
        ps_file.write(ps_script)
        ps_path = ps_file.name

    generated = 0
    for item in plan:
        out_path = Path(item["path"])
        out_path.parent.mkdir(parents=True, exist_ok=True)

        group = item["group"]
        line_text = item["text"]
        use_ssml = False

        if group.startswith("monster"):
            rate = args.rate_monster
            voice_name = args.voice_name_monster
            escaped = html.escape(line_text)
            line_text = (
                "<speak version='1.0' xml:lang='en-US'><prosody rate='-20%' pitch='-20%'>"
                + escaped
                + "</prosody></speak>"
            )
            use_ssml = True
        elif group.startswith("panic_female"):
            rate = args.rate_panic_female
            voice_name = args.voice_name_npc_female
            escaped = html.escape(line_text)
            line_text = (
                "<speak version='1.0' xml:lang='en-US'><prosody rate='+15%' pitch='+12%' volume='x-loud'>"
                + escaped
                + "</prosody></speak>"
            )
            use_ssml = True
        elif group.startswith("panic_male"):
            rate = args.rate_panic_male
            voice_name = args.voice_name_npc_male
            escaped = html.escape(line_text)
            line_text = (
                "<speak version='1.0' xml:lang='en-US'><prosody rate='+10%' pitch='+4%' volume='x-loud'>"
                + escaped
                + "</prosody></speak>"
            )
            use_ssml = True
        elif group.startswith("npc_female"):
            rate = args.rate_npc_female
            voice_name = args.voice_name_npc_female
        elif group.startswith("npc_male"):
            rate = args.rate_npc_male
            voice_name = args.voice_name_npc_male
        else:
            rate = args.rate_npc_male
            voice_name = args.voice_name_npc_male

        cmd = [
            "powershell",
            "-NoProfile",
            "-ExecutionPolicy",
            "Bypass",
            "-File",
            ps_path,
            "-Text",
            line_text,
            "-OutPath",
            str(out_path),
            "-Rate",
            str(rate),
            "-VoiceName",
            voice_name or "",
            "-UseSsml",
            "1" if use_ssml else "0",
        ]
        result = subprocess.run(cmd, capture_output=True, text=True)
        if result.returncode != 0:
            print(f"[TTS] PowerShell TTS failed for {out_path.name}: {result.stderr.strip()}")
            try:
                Path(ps_path).unlink(missing_ok=True)
            except Exception:
                pass
            return False
        generated += 1

    print(f"[TTS] Engine: PowerShell System.Speech ({generated} files)")

    try:
        Path(ps_path).unlink(missing_ok=True)
    except Exception:
        pass

    return True


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate monster and NPC dialogue WAV files.")
    parser.add_argument("--voice-npc-female", type=int, default=0, help="Voice index for female NPC lines")
    parser.add_argument("--voice-npc-male", type=int, default=0, help="Voice index for male NPC lines")
    parser.add_argument("--voice-monster", type=int, default=0, help="Voice index for monster lines (default: 0)")
    parser.add_argument("--voice-name-npc-female", default="", help="Windows voice name for female NPC lines")
    parser.add_argument("--voice-name-npc-male", default="", help="Windows voice name for male NPC lines")
    parser.add_argument("--voice-name-monster", default="", help="Windows voice name for monster lines (optional)")
    parser.add_argument("--rate-npc-female", type=int, default=172, help="Female NPC speech rate")
    parser.add_argument("--rate-npc-male", type=int, default=160, help="Male NPC speech rate")
    parser.add_argument("--rate-panic-female", type=int, default=178, help="Female panic speech rate")
    parser.add_argument("--rate-panic-male", type=int, default=170, help="Male panic speech rate")
    parser.add_argument("--rate-monster", type=int, default=135, help="Monster speech rate")
    parser.add_argument("--dry-run", action="store_true", help="Only print planned files; do not synthesize")
    args = parser.parse_args()

    if not WORLD_CPP.exists():
        print(f"[TTS] Could not find source file: {WORLD_CPP}")
        return 1

    source = WORLD_CPP.read_text(encoding="utf-8")
    dialogue = _collect_dialogue(source)
    folders = _ensure_dirs()

    plan: List[Dict[str, str]] = []

    def add_plan(group: str, target_folder: Path, lines: List[str]) -> None:
        for idx, line in enumerate(lines, start=1):
            filename = f"{idx:02d}_{_slug(line)}.wav"
            plan.append(
                {
                    "group": group,
                    "text": line,
                    "path": str(target_folder / filename),
                }
            )

    add_plan("monster_taunts", folders["monster"], dialogue.get("monster_taunts", []))
    add_plan("monster_screams", folders["monster"], dialogue.get("monster_screams", []))
    add_plan("npc_female_general", folders["npc_female"], dialogue.get("npc_general", []))
    add_plan("npc_female_neighbor", folders["npc_female"], dialogue.get("npc_neighbor", []))
    add_plan("npc_male_general", folders["npc_male"], dialogue.get("npc_general", []))
    add_plan("npc_male_neighbor", folders["npc_male"], dialogue.get("npc_neighbor", []))
    add_plan("npc_replies", folders["responses"], dialogue.get("reply_boyd", []))
    add_plan("npc_replies", folders["responses"], dialogue.get("reply_jade", []))
    add_plan("npc_replies", folders["responses"], dialogue.get("reply_tabitha", []))
    add_plan("npc_replies", folders["responses"], dialogue.get("reply_victor", []))
    add_plan("npc_replies", folders["responses"], dialogue.get("reply_sara", []))
    add_plan("monster_responses", folders["responses"], dialogue.get("monster_responses", []))
    add_plan("panic_female", folders["panic_female"], dialogue.get("panic", []))
    add_plan("panic_male", folders["panic_male"], dialogue.get("panic", []))

    manifest = {
        "source": str(WORLD_CPP),
        "output_root": str(OUT_ROOT),
        "entries": plan,
    }
    _save_manifest(manifest)

    print(f"[TTS] Parsed {len(plan)} dialogue lines.")
    print(f"[TTS] Manifest written to: {OUT_ROOT / 'manifest_dialogue_tts.json'}")

    if args.dry_run:
        for item in plan[:25]:
            print(f"  - {item['group']}: {item['path']}")
        if len(plan) > 25:
            print(f"  ... and {len(plan) - 25} more")
        print("[TTS] Dry-run complete.")
        return 0

    if _generate_with_pyttsx3(plan, args):
        print(f"[TTS] Generated {len(plan)} wav files under {OUT_ROOT}")
        return 0

    if _generate_with_powershell(plan, args):
        print(f"[TTS] Generated {len(plan)} wav files under {OUT_ROOT}")
        return 0

    print("[TTS] No usable TTS backend found.")
    print("[TTS] Options:")
    print("  1) Use Windows PowerShell with System.Speech (recommended on Windows)")
    print("  2) Install pyttsx3 and compatible voice backend")
    return 4


if __name__ == "__main__":
    raise SystemExit(main())
