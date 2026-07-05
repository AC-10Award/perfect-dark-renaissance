#!/usr/bin/env python3
"""Audit the material/impact ownership registry."""

from __future__ import annotations

import argparse
import csv
import re
import sys
from pathlib import Path


EXPECTED_COLUMNS = [
    "event",
    "material_category",
    "stage_scope",
    "legacy_texture_ids",
    "code_owner",
    "sfx_group",
    "texture_group",
    "status",
    "notes",
]

KNOWN_STATUSES = {
    "ACTIVE_DOC",
    "ARCHIVE",
    "NEEDS_REVIEW",
    "TESTING",
    "WAITING_FOR_RUNTIME",
}

LEGACY_TEXTURE_ID_PATTERN = re.compile(r"0x[0-9a-fA-F]+")


def parse_registry(path: Path, expected_columns: list[str]) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as handle:
        reader = csv.DictReader(handle)

        if reader.fieldnames != expected_columns:
            raise ValueError(f"{path} header is {reader.fieldnames}; expected {expected_columns}")

        return list(reader)


def registry_values(rows: list[dict[str, str]], column: str) -> set[str]:
    return {
        row[column].strip()
        for row in rows
        if row[column].strip()
    }


def split_groups(value: str) -> list[str]:
    return [part.strip() for part in value.split("|") if part.strip()]


def valid_legacy_texture_ids(value: str) -> bool:
    if value == "N/A":
        return True

    ids = split_groups(value)

    return bool(ids) and all(LEGACY_TEXTURE_ID_PATTERN.fullmatch(item) for item in ids)


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
    registry_path = root / "docs/references/material_impact_registry.csv"
    sfx_registry_path = root / "docs/references/sfx_event_registry.csv"
    effects_registry_path = root / "docs/references/effects_texture_registry.csv"

    rows = parse_registry(registry_path, EXPECTED_COLUMNS)
    sfx_rows = parse_registry(
        sfx_registry_path,
        ["alias", "filename", "event_group", "current_trigger", "status", "notes"],
    )
    effects_rows = parse_registry(
        effects_registry_path,
        [
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
        ],
    )

    known_sfx_groups = registry_values(sfx_rows, "event_group")
    known_texture_groups = registry_values(effects_rows, "variant")

    registry_events = [row["event"].strip() for row in rows]
    duplicate_events = sorted({
        event for event in registry_events
        if event and registry_events.count(event) > 1
    })

    problems: list[str] = []

    for index, row in enumerate(rows, start=2):
        event = row["event"].strip()
        status = row["status"].strip()
        code_owner = row["code_owner"].strip()
        legacy_texture_ids = row["legacy_texture_ids"].strip()
        sfx_groups = split_groups(row["sfx_group"].strip())
        texture_groups = split_groups(row["texture_group"].strip())

        if not event:
            problems.append(f"line {index}: missing event")

        if not row["material_category"].strip():
            problems.append(f"line {index}: missing material_category")

        if not row["stage_scope"].strip():
            problems.append(f"line {index}: missing stage_scope")

        if status not in KNOWN_STATUSES:
            problems.append(f"line {index}: unknown status {status!r}")

        if not code_owner:
            problems.append(f"line {index}: missing code_owner")
        elif code_owner != "N/A" and not (root / code_owner).is_file():
            problems.append(f"line {index}: code_owner does not exist: {code_owner}")

        if not valid_legacy_texture_ids(legacy_texture_ids):
            problems.append(
                f"line {index}: legacy_texture_ids must be N/A or pipe-separated hex ids: {legacy_texture_ids!r}"
            )

        if not sfx_groups:
            problems.append(f"line {index}: missing sfx_group")

        for sfx_group in sfx_groups:
            if sfx_group not in known_sfx_groups:
                problems.append(f"line {index}: unknown sfx_group {sfx_group!r}")

        if not texture_groups:
            problems.append(f"line {index}: missing texture_group")

        for texture_group in texture_groups:
            if texture_group not in known_texture_groups:
                problems.append(f"line {index}: unknown texture_group {texture_group!r}")

    print(f"material impact audit: {registry_path.relative_to(root)}")
    print(f"rows: {len(rows)}")

    print_section(
        "Registry problems",
        [
            *(f"duplicate event: {event}" for event in duplicate_events),
            *problems,
        ],
    )

    return 1 if duplicate_events or problems else 0


if __name__ == "__main__":
    sys.exit(main())
