#!/usr/bin/env python3
"""Structural check for the screenshot generator's output.

Runs `client_screenshot_generator` on the Qt offscreen platform into a
scratch directory, then verifies that every manifest-managed capture the
fixture lists was actually produced, and — where the fixture pins exact
dimensions — that the PNG matches them pixel-exact (offscreen renders at
DPR=1, so specs translate 1:1).

This is the cheap regression net for the docs pipeline: a capture that
silently disappears, or a view that renders at the wrong size, fails the
test suite instead of being discovered the next time someone refreshes the
web manual. Content-level checks (non-empty tables) live in the generator
itself.

It also fails a run in which two captures produced identical bytes, which
is one capture rendering another's picture under its own name. That shipped
for as long as the gallery has been tracked: graph-cursor.png and
limits-chart.png were two fixture rows differing only in `filename`, so
both plotted the one shared graph configuration and were the same file.
Nothing could see it — an image that renders is an image that passes, the
capture-review sheet only ever pairs a Qt capture against a web one, and
two captures of one capability are supposed to differ.

Registered as a ctest test by tools/screenshot_generator/CMakeLists.txt;
run manually with:

    python3 check_screenshots.py --generator <path-to-binary> \
        --data screenshot_data.json --image-manifest ../../screenshots/image_manifest.json
"""

import argparse
import hashlib
import json
import os
import struct
import subprocess
import sys
import tempfile
from pathlib import Path


def png_dimensions(path: Path) -> tuple[int, int]:
    """Reads width/height from the PNG IHDR chunk (no image libs needed)."""
    with open(path, "rb") as f:
        header = f.read(24)
    if len(header) < 24 or header[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError(f"{path} is not a PNG")
    width, height = struct.unpack(">II", header[16:24])
    return width, height


def duplicate_renders(paths: list[Path]) -> list[list[Path]]:
    """Groups produced captures by content digest, returning the collisions.

    Byte equality is the whole test: two captures of one capability are
    meant to be different views of it, so an exact match is never a near
    miss to be tolerated — it is one spec rendering the other's picture.
    """
    by_digest: dict[str, list[Path]] = {}
    for path in paths:
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
        by_digest.setdefault(digest, []).append(path)
    return [group for group in by_digest.values() if len(group) > 1]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--generator", type=Path, required=True)
    parser.add_argument("--data", type=Path, required=True)
    parser.add_argument("--image-manifest", type=Path, required=True)
    parser.add_argument(
        "--out",
        type=Path,
        default=None,
        help="Render directory (default: a fresh temp dir)",
    )
    args = parser.parse_args()

    out_dir = args.out or Path(tempfile.mkdtemp(prefix="screenshot-check-"))
    out_dir.mkdir(parents=True, exist_ok=True)

    env = dict(os.environ)
    env["QT_QPA_PLATFORM"] = "offscreen"

    try:
        result = subprocess.run(
            [
                str(args.generator),
                f"--out={out_dir}",
                f"--image-manifest={args.image_manifest}",
            ],
            env=env,
            cwd=args.generator.parent,
            timeout=240,
        )
    except subprocess.TimeoutExpired:
        print("error: generator timed out after 240s", file=sys.stderr)
        return 1
    if result.returncode != 0:
        print(
            f"error: generator exited with {result.returncode}",
            file=sys.stderr,
        )
        return 1

    data = json.loads(args.data.read_text(encoding="utf-8"))
    manifest = json.loads(args.image_manifest.read_text(encoding="utf-8"))
    managed = {
        e["file"] for e in manifest["images"] if e["tag"].startswith("auto-")
    }

    # Dimensions are enforced for view captures only: SaveScreenshot hard-
    # resizes the widget, so spec dims are exact on every platform. Dialogs
    # size to their layout minimums, which vary with platform font metrics
    # (the login dialog is 403px wide on Windows, 475px on macOS), so for
    # them only existence is checked.
    specs = [(s, True) for s in data.get("screenshots", [])] + [
        (s, False) for s in data.get("dialogs", [])
    ]
    errors = []
    produced = []
    checked = 0
    for spec, exact_dims in specs:
        filename = spec["filename"]
        if filename not in managed:
            continue
        path = out_dir / filename
        if not path.is_file():
            errors.append(f"{filename}: not produced")
            continue
        checked += 1
        produced.append(path)
        width = spec.get("width")
        height = spec.get("height")
        if exact_dims and width and height:
            actual = png_dimensions(path)
            if actual != (width, height):
                errors.append(
                    f"{filename}: {actual[0]}x{actual[1]}, spec says "
                    f"{width}x{height}"
                )

    for group in duplicate_renders(produced):
        names = ", ".join(sorted(p.name for p in group))
        errors.append(
            f"identical bytes, so one of them renders the other's picture: "
            f"{names}"
        )

    for error in errors:
        print(f"error: {error}", file=sys.stderr)
    print(f"{checked} captures checked in {out_dir}, {len(errors)} error(s)")
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
