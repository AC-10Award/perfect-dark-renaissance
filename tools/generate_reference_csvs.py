#!/usr/bin/env python3
import csv
import json
import re
from collections import defaultdict
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "src"
LOGS = ROOT / "Stress Test Gamelogs"
OUTDIR = ROOT / "docs" / "references"


def load_text(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def parse_define_map(path: Path, prefixes):
    pattern = re.compile(r"#define\s+([A-Z0-9_]+)\s+(0x[0-9a-fA-F]+|\d+)")
    values = {}

    for match in pattern.finditer(load_text(path)):
        name = match.group(1)

        if prefixes and not any(name.startswith(prefix) for prefix in prefixes):
            continue

        if name not in values:
            values[name] = int(match.group(2), 0)

    return values


def parse_lang_catalog():
    catalogs = {}

    for name in ("options", "mpweapons", "misc"):
        items = json.loads(load_text(SRC / "assets" / "ntsc-1.0" / "lang" / f"{name}.json"))
        catalogs.update(
            {
                item["id"]: (item.get("en") or "").replace("\n", "").strip()
                for item in items
            }
        )

    return catalogs


def clean_enum_name(name: str) -> str:
    name = re.sub(r"^(BODY_|HEAD_|MPBODY_|MPHEAD_)", "", name)
    name = name.replace("_Z", "")
    name = name.replace("_GE", " GE")
    name = name.replace("_AF1", " AF1")
    name = name.replace("_CI", " CI")
    name = name.replace("_DD", " DD")
    name = name.replace("_A51", " A51")
    name = name.replace("_NSA", " NSA")
    name = name.replace("_G5", " G5")
    name = name.replace("_MP", " MP")
    name = name.replace("_", " ")
    return name.title()


def extract_array_lines(text: str, array_name: str):
    start = text.index(f"{array_name}[] = {{")
    end = text.index("\n};", start)
    return text[start:end].splitlines()


def extract_enum_lines(text: str, enum_name: str):
    start = text.index(f"enum {enum_name} {{")
    end = text.index("\n};", start)
    return text[start:end].splitlines()


def parse_mp_heads(path: Path):
    lines = extract_array_lines(load_text(path), "g_MpHeads")
    pattern = re.compile(r"/\*0x([0-9a-fA-F]+)\*/\s*\{\s*(HEAD_[A-Z0-9_]+)\s*,\s*([A-Z0-9_]+|\d+)\s*\}")
    rows = []

    for line in lines:
        match = pattern.search(line)
        if match:
            rows.append(
                {
                    "mphead_slot_hex": f"0x{int(match.group(1), 16):02X}",
                    "head_const": match.group(2),
                    "requirefeature": match.group(3),
                }
            )

    return rows


def parse_mp_bodies(path: Path):
    lines = extract_array_lines(load_text(path), "g_MpBodies")
    pattern = re.compile(
        r"/\*0x([0-9a-fA-F]+)\*/\s*\{\s*"
        r"(BODY_[A-Z0-9_]+)\s*,\s*"
        r"([A-Z0-9_]+)\s*,\s*"
        r"(HEAD_[A-Z0-9_]+|1000)\s*,\s*"
        r"([A-Z0-9_]+|\d+)\s*\}"
    )
    rows = []

    for line in lines:
        match = pattern.search(line)
        if match:
            rows.append(
                {
                    "mpbody_slot_hex": f"0x{int(match.group(1), 16):02X}",
                    "body_const": match.group(2),
                    "lang_id": match.group(3),
                    "head_const": match.group(4),
                    "requirefeature": match.group(5),
                }
            )

    return rows


def parse_heads_and_bodies(path: Path):
    pattern = re.compile(
        r"\{\s*/\*0x([0-9a-fA-F]+)\*/\s*"
        r"[01]\s*,\s*[01]\s*,\s*[01]\s*,\s*[A-Z0-9_]+\s*,\s*\d+\s*,\s*"
        r"([A-Z0-9_]+|\d+)\s*,\s*[^,]+\s*,\s*[^,]+\s*,\s*0\s*,\s*([A-Z0-9_]+|\d+)\s*\}"
    )
    rows = {}

    for match in pattern.finditer(load_text(path)):
        rows[int(match.group(1), 16)] = {
            "file_const": match.group(2),
            "hand_file_const": match.group(3),
        }

    return rows


def parse_model_texture_logs():
    model_textures = defaultdict(set)
    pattern = re.compile(r"headtex:\s+model\s+([0-9a-fA-F]{4})\s+\(([^)]+)\)")
    tex_pattern = re.compile(r"texturenum=([0-9a-fA-F]{4})")

    for path in sorted(LOGS.glob("*.log")):
        current_model = None

        try:
            lines = path.read_text(encoding="utf-8", errors="ignore").splitlines()
        except OSError:
            continue

        for line in lines:
            model_match = pattern.search(line)

            if model_match:
                current_model = int(model_match.group(1), 16)
                continue

            tex_match = tex_pattern.search(line)

            if tex_match and current_model is not None:
                model_textures[current_model].add(f"0x{int(tex_match.group(1), 16):04X}")

    return {key: ",".join(sorted(values)) for key, values in model_textures.items()}


def build_character_rows():
    constants = parse_define_map(SRC / "include" / "constants.h", ("HEAD_", "BODY_", "MPHEAD_", "MPBODY_", "MPFEATURE_"))
    files = parse_define_map(SRC / "include" / "files.h", ("FILE_",))
    lang = parse_lang_catalog()
    mp_heads = {row["head_const"]: row for row in parse_mp_heads(SRC / "game" / "mplayer" / "mplayer.c")}
    mp_bodies = parse_mp_bodies(SRC / "game" / "mplayer" / "mplayer.c")
    headbody = parse_heads_and_bodies(SRC / "game" / "modeldata" / "robot.c")
    textures = parse_model_texture_logs()

    rows = []

    for row in mp_bodies:
        body_const = row["body_const"]
        body_id = constants.get(body_const)
        body_meta = headbody.get(body_id, {})
        body_file_const = body_meta.get("file_const", "0")
        body_file_id = files.get(body_file_const)

        head_const = row["head_const"]
        if head_const == "1000":
            head_id = ""
            head_file_const = ""
            head_file_id = ""
            head_tex_ids = ""
            head_display = "Dynamic pool"
        else:
            head_id = constants.get(head_const, "")
            head_meta = headbody.get(head_id, {})
            head_file_const = head_meta.get("file_const", "")
            head_file_id = files.get(head_file_const, "")
            head_tex_ids = textures.get(head_file_id, "") if isinstance(head_file_id, int) else ""
            head_display = clean_enum_name(head_const)

        body_tex_ids = textures.get(body_file_id, "") if isinstance(body_file_id, int) else ""

        display_name = lang.get(row["lang_id"], "")
        if not display_name or display_name == "Dinner Jacket":
            display_name = clean_enum_name(body_const)

        code_ids = [
            f"mpbody_slot={row['mpbody_slot_hex']}",
            f"mpbody_const=MP{body_const}",
            f"body_const={body_const}",
            f"body_file={body_file_const}",
        ]

        if head_const == "1000":
            code_ids.append("head_const=1000(dynamic)")
        else:
            mp_head_slot = mp_heads.get(head_const, {}).get("mphead_slot_hex", "")
            if mp_head_slot:
                code_ids.append(f"mphead_slot={mp_head_slot}")
                code_ids.append(f"mphead_const=MP{head_const}")
            code_ids.append(f"head_const={head_const}")
            code_ids.append(f"head_file={head_file_const}")

        hand_file_const = body_meta.get("hand_file_const", "0")
        if hand_file_const and hand_file_const != "0":
            code_ids.append(f"hand_file={hand_file_const}")

        rows.append(
            {
                "CHARACTER NAME": display_name,
                "HEAD ID": f"0x{head_id:04X}" if isinstance(head_id, int) else "",
                "BODY ID": f"0x{body_id:04X}" if isinstance(body_id, int) else "",
                "Head Texture IDs": head_tex_ids,
                "Body Texture IDs": body_tex_ids,
                "Any computer code ID for the character": "; ".join(code_ids),
                "HEAD NAME": head_display,
                "BODY FILE ID": f"0x{body_file_id:04X}" if isinstance(body_file_id, int) else "",
                "HEAD FILE ID": f"0x{head_file_id:04X}" if isinstance(head_file_id, int) else "",
            }
        )

    return rows


def build_sfx_rows():
    lines = extract_enum_lines(load_text(SRC / "include" / "sfx.h"), "sfx")
    rows = []
    index = 0

    for line in lines:
        stripped = line.strip().rstrip(",")

        if not stripped or stripped.startswith("/") or stripped in {"enum sfx {", "};"}:
            continue

        if not stripped.startswith("SFX_"):
            continue

        rows.append(
            {
                "SFX NAME": stripped,
                "ID TAG": f"0x{index:04X}",
            }
        )
        index += 1

    return rows


def write_csv(path: Path, rows, fieldnames):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)


def main():
    character_rows = build_character_rows()
    sfx_rows = build_sfx_rows()

    write_csv(
        OUTDIR / "mp_character_reference.csv",
        character_rows,
        [
            "CHARACTER NAME",
            "HEAD ID",
            "BODY ID",
            "Head Texture IDs",
            "Body Texture IDs",
            "Any computer code ID for the character",
            "HEAD NAME",
            "BODY FILE ID",
            "HEAD FILE ID",
        ],
    )
    write_csv(
        OUTDIR / "sfx_reference.csv",
        sfx_rows,
        [
            "SFX NAME",
            "ID TAG",
        ],
    )

    print(f"Wrote {len(character_rows)} character rows to {OUTDIR / 'mp_character_reference.csv'}")
    print(f"Wrote {len(sfx_rows)} SFX rows to {OUTDIR / 'sfx_reference.csv'}")


if __name__ == "__main__":
    main()
