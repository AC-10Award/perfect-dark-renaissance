#!/usr/bin/env python3
"""Run the native asset ownership audits."""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path


AUDITS = [
    ("External music", "tools/audit_ext_music.py"),
    ("External SFX", "tools/audit_ext_sfx.py"),
    ("Font textures", "tools/audit_font_textures.py"),
    ("Texture registries", "tools/audit_texture_registry.py"),
    ("Material impact registry", "tools/audit_material_impact.py"),
]


def run_audit(root: Path, title: str, script: str) -> int:
    print(f"\n=== {title} ===", flush=True)
    result = subprocess.run(
        [sys.executable, str(root / script), "--root", str(root)],
        cwd=root,
        check=False,
    )
    return result.returncode


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
    failures: list[str] = []

    print(f"native asset audit: {root}", flush=True)

    for title, script in AUDITS:
        exit_code = run_audit(root, title, script)

        if exit_code != 0:
            failures.append(title)

    print("\n=== Summary ===", flush=True)

    if failures:
        print("FAILED:")
        for title in failures:
            print(f"- {title}")
        return 1

    print("All native asset audits passed.")
    print("Optional build check: cmake --build build --target pd -j4")
    return 0


if __name__ == "__main__":
    sys.exit(main())
