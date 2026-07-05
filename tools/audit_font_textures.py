#!/usr/bin/env python3
"""Audit external HD font texture folders."""

from __future__ import annotations

import argparse
import struct
import sys
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path


FONT_DIR_NAMES = {
    "fonthandelgothicsm",
    "fonthandelgothicmd",
    "fonthandelgothicxs",
    "fonthandelgothiclg",
    "fontnumeric",
}

PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"
MAX_FONT_GLYPHS = 94
IGNORED_FILENAMES = {
    "readme.md",
    "readme.txt",
    ".ds_store",
}


@dataclass(frozen=True)
class PngInfo:
    width: int
    height: int
    bit_depth: int
    color_type: int


def parse_font_glyph_id(path: Path) -> int | None:
    stem = path.stem
    prefix = []

    for char in stem:
        if char.lower() not in "0123456789abcdef":
            break

        prefix.append(char)

    if prefix:
        try:
            return int("".join(prefix), 16)
        except ValueError:
            return None

    if len(stem) >= 3 and stem[-3] == "-":
        suffix = stem[-2:]

        if all(char.lower() in "0123456789abcdef" for char in suffix):
            return int(suffix, 16)

    return None


def read_png_info(path: Path) -> PngInfo:
    with path.open("rb") as handle:
        header = handle.read(33)

    if len(header) < 33 or not header.startswith(PNG_SIGNATURE):
        raise ValueError("not a PNG file")

    length, chunk_type = struct.unpack(">I4s", header[8:16])

    if length != 13 or chunk_type != b"IHDR":
        raise ValueError("missing PNG IHDR")

    width, height, bit_depth, color_type = struct.unpack(">IIBB", header[16:26])

    return PngInfo(width=width, height=height, bit_depth=bit_depth, color_type=color_type)


def color_type_name(color_type: int) -> str:
    return {
        0: "grayscale",
        2: "rgb",
        3: "indexed",
        4: "grayscale-alpha",
        6: "rgba",
    }.get(color_type, f"unknown-{color_type}")


def is_auxiliary_dir(path: Path) -> bool:
    return path.name in {"outlines", "unmapped"} or path.name.startswith(".")


def collect_font_dirs(root: Path, requested: list[Path]) -> list[Path]:
    if requested:
        return [path if path.is_absolute() else root / path for path in requested]

    fonts_root = root / "mods/mod_allinone/ext_tex/fonts"

    if not fonts_root.is_dir():
        return []

    return sorted(path for path in fonts_root.iterdir() if path.is_dir() and path.name in FONT_DIR_NAMES)


def audit_glyph_dir(path: Path, label: str) -> tuple[int, list[str]]:
    problems: list[str] = []
    warnings: list[str] = []
    ids: dict[int, list[Path]] = defaultdict(list)
    dimensions: dict[tuple[int, int], list[Path]] = defaultdict(list)
    formats: dict[tuple[int, int], list[Path]] = defaultdict(list)

    for file_path in sorted(path.iterdir()):
        if file_path.is_dir():
            if not is_auxiliary_dir(file_path):
                warnings.append(f"{label}: unexpected subdirectory {file_path.name}/")
            continue

        if file_path.name.startswith(".") or file_path.name.lower() in IGNORED_FILENAMES:
            continue

        if file_path.suffix.lower() != ".png":
            warnings.append(f"{label}: non-PNG file ignored: {file_path.name}")
            continue

        glyph_id = parse_font_glyph_id(file_path)

        if glyph_id is None:
            problems.append(f"{label}: unmapped PNG filename: {file_path.name}")
        elif glyph_id < 0 or glyph_id >= MAX_FONT_GLYPHS:
            problems.append(f"{label}: glyph id {glyph_id:02x} outside 00-5d: {file_path.name}")
        else:
            ids[glyph_id].append(file_path)

        try:
            info = read_png_info(file_path)
        except ValueError as exc:
            problems.append(f"{label}: unreadable PNG {file_path.name}: {exc}")
            continue

        dimensions[(info.width, info.height)].append(file_path)
        formats[(info.bit_depth, info.color_type)].append(file_path)

        if info.color_type != 6:
            warnings.append(
                f"{label}: {file_path.name} is {color_type_name(info.color_type)}, expected rgba"
            )

    for glyph_id, paths in sorted(ids.items()):
        if len(paths) > 1:
            names = ", ".join(path.name for path in paths)
            problems.append(f"{label}: duplicate glyph {glyph_id:02x}: {names}")

    if len(dimensions) > 1:
        summary = ", ".join(
            f"{width}x{height}={len(paths)}" for (width, height), paths in sorted(dimensions.items())
        )
        warnings.append(f"{label}: mixed canvas sizes: {summary}")

    if len(formats) > 1:
        summary = ", ".join(
            f"{bit_depth}-bit {color_type_name(color_type)}={len(paths)}"
            for (bit_depth, color_type), paths in sorted(formats.items())
        )
        warnings.append(f"{label}: mixed PNG formats: {summary}")

    print_section(f"{label} problems", problems)
    print_section(f"{label} warnings", warnings)

    return (1 if problems else 0), problems + warnings


def print_section(title: str, rows: list[str]) -> None:
    print(f"\n{title}")
    print("-" * len(title))

    if rows:
        for row in rows:
            print(row)
    else:
        print("OK")


def audit_font_dir(path: Path, root: Path) -> int:
    if not path.is_dir():
        print(f"\n{path}: missing directory")
        return 1

    try:
        label = str(path.relative_to(root))
    except ValueError:
        label = str(path)

    exit_code, _ = audit_glyph_dir(path, label)

    outlines = path / "outlines"
    if outlines.is_dir():
        outline_label = f"{label}/outlines"
        outline_exit, _ = audit_glyph_dir(outlines, outline_label)
        exit_code |= outline_exit

    return exit_code


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--root",
        type=Path,
        default=Path(__file__).resolve().parents[1],
        help="Perfect Dark Renaissance root directory",
    )
    parser.add_argument(
        "font_dirs",
        nargs="*",
        type=Path,
        help="font folder(s), relative to root or absolute",
    )
    args = parser.parse_args()

    root = args.root.resolve()
    font_dirs = collect_font_dirs(root, args.font_dirs)

    print("font texture audit")
    print(f"root: {root}")
    print(f"font dirs: {len(font_dirs)}")

    exit_code = 0

    for font_dir in font_dirs:
        exit_code |= audit_font_dir(font_dir.resolve(), root)

    return exit_code


if __name__ == "__main__":
    sys.exit(main())
