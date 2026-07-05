#!/usr/bin/env python3
"""Audit texture ownership registry CSV files."""

from __future__ import annotations

import argparse
import csv
import sys
from collections import defaultdict
from pathlib import Path


EXPECTED_COLUMNS = [
    "concept",
    "variant",
    "engine_slot",
    "current_model_file",
    "texture_id",
    "current_path",
    "body_region",
    "status",
    "ownership_scope",
    "runtime_note",
    "notes",
]

KNOWN_STATUSES = {
    "ACTIVE_FILE",
    "TESTING",
    "NEEDS_REVIEW",
    "WAITING_FOR_ART",
    "ARCHIVE",
}


def parse_registry(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as handle:
        reader = csv.DictReader(handle)

        if reader.fieldnames != EXPECTED_COLUMNS:
            raise ValueError(f"{path} header is {reader.fieldnames}; expected {EXPECTED_COLUMNS}")

        return list(reader)


def has_testing_suffix(path: str) -> bool:
    return Path(path).stem.lower().endswith("_testing")


def leading_hex(path: str) -> str:
    stem = Path(path).stem
    chars: list[str] = []

    for char in stem:
        if char.lower() not in "0123456789abcdef":
            break
        chars.append(char)

    return "".join(chars).lower()


def print_section(title: str, rows: list[str]) -> None:
    print(f"\n{title}")
    print("-" * len(title))

    if rows:
        for row in rows:
            print(row)
    else:
        print("OK")


def audit_registry(root: Path, registry_path: Path) -> int:
    rows = parse_registry(registry_path)
    problems: list[str] = []
    missing_files: list[str] = []
    testing_mismatches: list[str] = []
    duplicate_ids: list[str] = []
    prefix_mismatches: list[str] = []
    ids: dict[str, list[int]] = defaultdict(list)

    for index, row in enumerate(rows, start=2):
        texture_id = row["texture_id"].strip().lower()
        current_path = row["current_path"].strip()
        status = row["status"].strip()
        resolved_path = root / current_path

        if not texture_id:
            problems.append(f"line {index}: missing texture_id")
        else:
            ids[texture_id].append(index)

        if status not in KNOWN_STATUSES:
            problems.append(f"line {index}: unknown status {status!r}")

        if not current_path:
            problems.append(f"line {index}: missing current_path")
        elif not resolved_path.is_file():
            missing_files.append(f"line {index}: {current_path}")

        if current_path:
            path_hex = leading_hex(current_path)

            if texture_id and path_hex and path_hex != texture_id:
                prefix_mismatches.append(
                    f"line {index}: texture_id {texture_id} does not match filename prefix {path_hex}: {current_path}"
                )

        is_testing_file = has_testing_suffix(current_path)

        if is_testing_file and status != "TESTING":
            testing_mismatches.append(
                f"line {index}: _Testing filename should normally use TESTING status: {current_path} ({status})"
            )

        if status == "TESTING" and not is_testing_file:
            testing_mismatches.append(
                f"line {index}: TESTING status should use a _Testing filename: {current_path}"
            )

        if status == "ACTIVE_FILE" and current_path and not resolved_path.is_file():
            problems.append(f"line {index}: ACTIVE_FILE is missing on disk: {current_path}")

    for texture_id, line_numbers in sorted(ids.items()):
        if len(line_numbers) > 1:
            duplicate_ids.append(
                f"texture_id {texture_id}: lines {', '.join(str(line) for line in line_numbers)}"
            )

    print(f"texture registry audit: {registry_path.relative_to(root)}")
    print(f"rows: {len(rows)}")

    print_section("Registry problems", problems)
    print_section("Missing files", missing_files)
    print_section("Duplicate texture IDs", duplicate_ids)
    print_section("Filename/status testing mismatches", testing_mismatches)
    print_section("Texture ID / filename prefix mismatches", prefix_mismatches)

    return 1 if problems or missing_files or duplicate_ids or testing_mismatches or prefix_mismatches else 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--root",
        type=Path,
        default=Path(__file__).resolve().parents[1],
        help="Perfect Dark Renaissance root directory",
    )
    parser.add_argument(
        "registries",
        nargs="*",
        type=Path,
        help="registry CSV path(s), relative to root or absolute",
    )
    args = parser.parse_args()

    root = args.root.resolve()
    registries = args.registries or [
        Path("docs/references/termination_texture_registry.csv"),
        Path("docs/references/effects_texture_registry.csv"),
    ]

    exit_code = 0

    for registry in registries:
        registry_path = registry if registry.is_absolute() else root / registry
        exit_code |= audit_registry(root, registry_path)

    return exit_code


if __name__ == "__main__":
    sys.exit(main())
