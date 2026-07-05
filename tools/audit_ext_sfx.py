#!/usr/bin/env python3
"""Audit ext_sfx aliases, registry rows, and WAV files."""

from __future__ import annotations

import argparse
import csv
import re
import sys
from pathlib import Path


EXPECTED_COLUMNS = [
    "alias",
    "filename",
    "event_group",
    "current_trigger",
    "status",
    "notes",
]

KNOWN_STATUSES = {
    "ACTIVE_FILE",
    "ARCHIVE",
    "TESTING",
    "WAITING_FOR_ASSET",
}

ARCHIVE_FILES = {
    "falcon_silencer_on.wav",
    "door_explosion_debris (older).wav",
    "tile_hit_01 (oringla0.wav",
}


def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def parse_aliases(path: Path) -> dict[str, str]:
    aliases: dict[str, str] = {}
    pattern = re.compile(r'#define\s+(MD_SFX_[A-Z0-9_]+)\s+"([^"]+\.wav)"')

    for match in pattern.finditer(read_text(path)):
        aliases[match.group(1)] = match.group(2)

    return aliases


def parse_registry(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as handle:
        reader = csv.DictReader(handle)

        if reader.fieldnames != EXPECTED_COLUMNS:
            raise ValueError(f"{path} header is {reader.fieldnames}; expected {EXPECTED_COLUMNS}")

        return list(reader)


def wav_files(path: Path) -> set[str]:
    return {
        file.name
        for file in path.iterdir()
        if file.is_file() and file.suffix.lower() == ".wav"
    }


def print_section(title: str, rows: list[str]) -> None:
    print(f"\n{title}")
    print("-" * len(title))

    if rows:
        for row in rows:
            print(row)
    else:
        print("OK")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--root",
        type=Path,
        default=Path(__file__).resolve().parents[1],
        help="Perfect Dark Renaissance root directory",
    )
    args = parser.parse_args()

    root = args.root.resolve()
    alias_path = root / "src/include/md/audio_aliases.h"
    registry_path = root / "docs/references/sfx_event_registry.csv"
    sfx_path = root / "ext_sfx"

    aliases = parse_aliases(alias_path)
    rows = parse_registry(registry_path)
    files = wav_files(sfx_path)

    registry_aliases = [row["alias"].strip() for row in rows]
    registry_filenames = [row["filename"].strip() for row in rows]
    registry_alias_set = set(registry_aliases)
    registry_file_set = set(registry_filenames)
    alias_file_set = set(aliases.values())

    duplicate_aliases = sorted({
        alias for alias in registry_alias_set
        if registry_aliases.count(alias) > 1
    })
    duplicate_filenames = sorted({
        filename for filename in registry_file_set
        if registry_filenames.count(filename) > 1
    })

    problems: list[str] = []
    missing_files: list[str] = []

    for index, row in enumerate(rows, start=2):
        alias = row["alias"].strip()
        filename = row["filename"].strip()
        status = row["status"].strip()

        if status not in KNOWN_STATUSES:
            problems.append(f"line {index}: unknown status {status!r}")

        if alias not in aliases:
            problems.append(f"line {index}: registry alias is not defined in audio_aliases.h: {alias}")
        elif aliases[alias] != filename:
            problems.append(
                f"line {index}: registry filename {filename} does not match alias {alias} -> {aliases[alias]}"
            )

        if status == "ACTIVE_FILE" and filename not in files:
            missing_files.append(f"line {index}: {filename}")

    alias_missing_registry = sorted(set(aliases) - registry_alias_set)
    registry_missing_alias = sorted(registry_alias_set - set(aliases))
    alias_files_missing = sorted(alias_file_set - files)
    extra_files = sorted(files - alias_file_set - ARCHIVE_FILES)
    archived_files_present = sorted(files & ARCHIVE_FILES)

    print(f"ext_sfx audit: {root}")
    print(f"aliases:        {len(aliases)}")
    print(f"registry rows:  {len(rows)}")
    print(f"wav files:      {len(files)}")

    print_section(
        "Registry problems",
        [
            *(f"duplicate registry alias: {alias}" for alias in duplicate_aliases),
            *(f"duplicate registry filename: {filename}" for filename in duplicate_filenames),
            *problems,
            *(f"alias missing from registry: {alias}" for alias in alias_missing_registry),
            *(f"registry alias missing from code: {alias}" for alias in registry_missing_alias),
        ],
    )
    print_section("Active alias files missing on disk", [f"{filename}" for filename in alias_files_missing])
    print_section("Active registry files missing on disk", missing_files)
    print_section("Unregistered live WAV files", [f"{filename}" for filename in extra_files])
    print_section("Recognized archive/source WAV files", [f"{filename}" for filename in archived_files_present])

    has_problem = bool(
        duplicate_aliases
        or duplicate_filenames
        or problems
        or alias_missing_registry
        or registry_missing_alias
        or alias_files_missing
        or missing_files
        or extra_files
    )

    return 1 if has_problem else 0


if __name__ == "__main__":
    sys.exit(main())
