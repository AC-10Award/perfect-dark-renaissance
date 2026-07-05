#!/usr/bin/env python3
"""Audit ext_music docs, runtime keys, and MP3 filenames."""

from __future__ import annotations

import argparse
import csv
import re
import sys
from pathlib import Path


RESERVED_TYPES = {"FUTURE_MUSIC", "AMBIENCE"}
ACTIVE_TYPES = {
    "PRIMARY",
    "MUSIC",
    "MENU",
    "RESULT",
    "DEATH",
    "CUSTOM_RESULT",
    "CUSTOM_EVENT",
}
LEGACY_FILES = {
    "menu.mp3": "legacy fallback for main_menu",
}
DEFAULT_IGNORED_DIRS = {"used_songs"}
TRACK_MAP_FILENAMES = (
    "MUSIC_TRACK_MAP_TEMPLATE.csv",
    "TRACK_MAP_TEMPLATE.csv",
)


def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def parse_aliases(path: Path) -> dict[str, str]:
    aliases: dict[str, str] = {}
    pattern = re.compile(r'#define\s+(MD_MUSIC_KEY_[A-Z0-9_]+)\s+"([^"]+)"')

    for match in pattern.finditer(read_text(path)):
        aliases[match.group(1)] = match.group(2)

    return aliases


def parse_runtime_keys(path: Path, aliases: dict[str, str]) -> list[str]:
    text = read_text(path)
    match = re.search(
        r"g_ExtMusicKnownKeys\[\]\s*=\s*\{(?P<body>.*?)\};",
        text,
        re.DOTALL,
    )

    if not match:
        raise ValueError(f"Could not find g_ExtMusicKnownKeys in {path}")

    keys: list[str] = []
    for token in re.findall(r"\bMD_MUSIC_KEY_[A-Z0-9_]+\b", match.group("body")):
        if token not in aliases:
            raise ValueError(f"{token} is listed at runtime but has no alias definition")
        keys.append(aliases[token])

    return keys


def find_track_map(ext_music: Path) -> Path:
    for filename in TRACK_MAP_FILENAMES:
        path = ext_music / filename

        if path.is_file():
            return path

    expected = ", ".join(TRACK_MAP_FILENAMES)
    raise FileNotFoundError(f"missing external music track map; expected one of: {expected}")


def parse_track_map(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as handle:
        reader = csv.DictReader(handle)
        expected_headers = [
            "event_key",
            "track_type",
            "context_or_source",
            "current_filename_rule",
            "notes",
        ]
        current_headers = [
            "event_key",
            "track_type",
            "context_or_source",
            "original_filename",
            "CURRENT FILENAME",
            "New UI Track Entry",
            "notes",
        ]

        if reader.fieldnames not in (expected_headers, current_headers):
            raise ValueError(
                f"{path} header is {reader.fieldnames}; expected {current_headers} or {expected_headers}"
            )

        return list(reader)


def mp3_files(root: Path, ignored_dirs: set[str]) -> list[Path]:
    files: list[Path] = []

    for path in root.rglob("*.mp3"):
        if not path.is_file():
            continue

        relative_parts = set(path.relative_to(root).parts[:-1])

        if relative_parts & ignored_dirs:
            continue

        files.append(path)

    return sorted(files)


def filename_matches_key(filename: str, key: str, longer_keys: set[str]) -> bool:
    lower = filename.lower()
    key_lower = key.lower()

    if not lower.endswith(".mp3"):
        return False

    stem = lower[:-4]

    if stem == key_lower:
        return True

    if len(stem) <= len(key_lower) + 1:
        return False

    if not stem.startswith(key_lower):
        return False

    if stem[len(key_lower)] not in {"_", "-"}:
        return False

    for longer in longer_keys:
        longer_lower = longer.lower()

        if len(longer_lower) <= len(key_lower):
            continue

        if len(longer_lower) > len(stem):
            continue

        if stem.startswith(longer_lower):
            if len(stem) == len(longer_lower) or stem[len(longer_lower)] in {"_", "-"}:
                return False

    return True


def classify_files(files: list[Path], keys: set[str]) -> tuple[dict[str, list[Path]], list[Path]]:
    matched: dict[str, list[Path]] = {key: [] for key in keys}
    unmatched: list[Path] = []

    for path in files:
        filename = path.name
        matches = [
            key for key in keys
            if filename_matches_key(filename, key, keys)
        ]

        if matches:
            matched[max(matches, key=len)].append(path)
        elif filename not in LEGACY_FILES:
            unmatched.append(path)

    return matched, unmatched


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
    parser.add_argument(
        "--include-used-songs",
        action="store_true",
        help="also audit ext_music/used_songs archive/source MP3 files",
    )
    args = parser.parse_args()

    root = args.root.resolve()
    ext_music = root / "ext_music"
    alias_path = root / "src/include/md/audio_aliases.h"
    runtime_path = root / "port/src/ext_music.c"
    track_map_path = find_track_map(ext_music)

    aliases = parse_aliases(alias_path)
    runtime_keys = parse_runtime_keys(runtime_path, aliases)
    rows = parse_track_map(track_map_path)

    doc_keys = [row["event_key"] for row in rows]
    doc_key_set = set(doc_keys)
    runtime_key_set = set(runtime_keys)
    active_doc_keys = {
        row["event_key"]
        for row in rows
        if row["track_type"] in ACTIVE_TYPES
    }
    reserved_doc_keys = {
        row["event_key"]
        for row in rows
        if row["track_type"] in RESERVED_TYPES
    }

    ignored_dirs = set() if args.include_used_songs else DEFAULT_IGNORED_DIRS
    files = mp3_files(ext_music, ignored_dirs)
    file_keys = doc_key_set | runtime_key_set
    matched_files, unmatched_files = classify_files(files, file_keys)

    duplicate_docs = sorted(
        key for key in doc_key_set
        if doc_keys.count(key) > 1
    )
    runtime_missing_docs = sorted(runtime_key_set - doc_key_set)
    active_docs_missing_runtime = sorted(active_doc_keys - runtime_key_set)
    reserved_docs_already_runtime = sorted(reserved_doc_keys & runtime_key_set)
    unknown_track_types = sorted(
        {
            row["track_type"]
            for row in rows
            if row["track_type"] not in ACTIVE_TYPES | RESERVED_TYPES
        }
    )
    active_keys_missing_files = sorted(
        key for key in active_doc_keys
        if not matched_files.get(key)
    )
    reserved_keys_with_files = sorted(
        key for key in reserved_doc_keys
        if matched_files.get(key)
    )

    print(f"ext_music audit: {root}")
    print(f"documented keys: {len(doc_key_set)}")
    print(f"runtime keys:    {len(runtime_key_set)}")
    print(f"mp3 files:       {len(files)}")
    if ignored_dirs:
        print(f"ignored dirs:    {', '.join(sorted(ignored_dirs))}")

    print_section(
        "Documentation problems",
        [
            *(f"duplicate event_key: {key}" for key in duplicate_docs),
            *(f"unknown track_type: {track_type}" for track_type in unknown_track_types),
            *(f"runtime key missing from {track_map_path.name}: {key}" for key in runtime_missing_docs),
            *(f"active documented key is not wired at runtime: {key}" for key in active_docs_missing_runtime),
            *(f"reserved key is already wired at runtime: {key}" for key in reserved_docs_already_runtime),
        ],
    )

    print_section(
        "Active keys without MP3 files",
        [f"{key}" for key in active_keys_missing_files],
    )

    print_section(
        "Reserved keys with MP3 files",
        [f"{key}" for key in reserved_keys_with_files],
    )

    print_section(
        "MP3 files that do not match documented or runtime keys",
        [str(path.relative_to(root)) for path in unmatched_files],
    )

    if any(LEGACY_FILES.get(path.name) for path in files):
        print_section(
            "Recognized legacy files",
            [
                f"{path.relative_to(root)}: {LEGACY_FILES[path.name]}"
                for path in files
                if path.name in LEGACY_FILES
            ],
        )

    has_doc_problem = bool(
        duplicate_docs
        or unknown_track_types
        or runtime_missing_docs
        or active_docs_missing_runtime
        or reserved_docs_already_runtime
    )

    return 1 if has_doc_problem else 0


if __name__ == "__main__":
    sys.exit(main())
