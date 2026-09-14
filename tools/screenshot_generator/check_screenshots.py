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

It reports its own **scope**, because it does not cover the gallery and a
clean run reads as though it does. The summary line carries a denominator, and
beneath it the rows this pass verified nothing about.

This used to be two ctests. A second one, `client_screenshot_check_themed`, ran
this same file under `--theme` over the manifest rows tagged `reshell-theme` --
the captures that existed only when the opt-in design-token theme was on. There
is no opt-in any more and no un-themed appearance to be the other half, so those
rows are ordinary `auto-view` / `auto-dialog` rows and one pass covers them all.
`--theme` survives only to render the check under `light` or `hc` instead of the
generator's default.

It also reports the **owed set**: manifest rows the docs pipeline manages
(`auto-*`) that no capture in this generator produces. That is the remaining
work of task 39, and it is reported rather than failed on — a non-zero count is
the backlog, not a regression. `--report-owed` prints it without running the
generator at all, so the backlog can cite a command instead of a number that
goes stale (it went stale twice; see task 374).

Registered as a ctest test by tools/screenshot_generator/CMakeLists.txt;
run manually with:

    python3 check_screenshots.py --generator <path-to-binary>
    python3 check_screenshots.py --report-owed      # no build needed
"""

import argparse
import hashlib
import json
import os
import re
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


def hardcoded_capture_filenames(source_dir: Path) -> set[str]:
    """PNG names written straight into the generator's C++ sources.

    Not every capture comes from screenshot_data.json: the standalone ones
    (`CaptureSettingsPanel`, `CaptureMoreMenu`, …) name their file in a
    `constexpr const char* kFilename` beside the test. A row rendered that way
    is not owed, and reading the manifest alone reports it as if it were —
    which is exactly how the count in task 39 was wrong about
    settings-dialog.png for months.
    """
    names: set[str] = set()
    for path in sorted(source_dir.glob("*.cpp")) + sorted(source_dir.glob("*.h")):
        text = path.read_text(encoding="utf-8", errors="replace")
        names.update(re.findall(r'"([\w.\-]+\.png)"', text))
    return names


# The appearance whose renders carry the bare filename from
# `screenshot_data.json`. Mirrors `kUnsuffixedTheme` in screenshot_output.cpp,
# and the two have to agree: that file decides what the generator WRITES and
# this one decides what is looked for. Dark is unsuffixed because dark is what
# the client starts in.
GALLERY_DEFAULT_THEME = "dark"


def themed_name(filename: str, theme: str) -> str:
    """The gallery filename a spec renders to under `theme`.

    The Python twin of `ThemedOutputPath`. Kept deliberately small, and the
    generator's own unit tests are the ones that pin the rule.
    """
    if theme == GALLERY_DEFAULT_THEME:
        return filename
    stem, _, ext = filename.rpartition(".")
    return f"{stem}-{theme}.{ext}"


def gallery_themes(manifest: dict) -> list[str]:
    """The appearances the manifest says the gallery holds.

    Read from each row's `theme` field, so adding `hc` captures is a manifest
    change and not a code change here. Rows without one are the default
    appearance.

    DECLARED rather than inferred from the filename, which was tried first and
    is wrong: stripping a trailing `-<suffix>` and asking whether the remainder
    is a real capture reads `devices-create.png` -- the create-object menu over
    the Objects tree -- as a "create"-themed variant of `devices.png`, because
    both of those files genuinely exist. It then counts that row as producible
    and stops reporting it as owed, which is the failure that matters: a
    capture nobody renders silently leaves the owed list.
    """
    themes = {GALLERY_DEFAULT_THEME}
    for entry in manifest["images"]:
        if entry["tag"].startswith("auto-") and entry.get("theme"):
            themes.add(entry["theme"])
    return sorted(themes)


def owed_captures(
    manifest: dict, data: dict, source_dir: Path
) -> dict[str, list[str]]:
    """Manifest-managed rows no capture in this generator produces, by tag.

    "Managed" is the `auto-*` prefix, the same predicate the existence check
    below and validate_image_manifest.py use.
    """
    rendered = {
        spec["filename"]
        for key in ("screenshots", "dialogs")
        for spec in data.get(key, [])
    } | hardcoded_capture_filenames(source_dir)

    # A themed variant is produced by the same spec under another `--theme`,
    # so it is owed only when its base capture is.
    producible = {
        themed_name(name, theme)
        for name in rendered
        for theme in gallery_themes(manifest)
    }

    owed: dict[str, list[str]] = {}
    for entry in manifest["images"]:
        if not entry["tag"].startswith("auto-"):
            continue
        if entry["file"] in producible:
            continue
        owed.setdefault(entry["tag"], []).append(entry["file"])
    return {tag: sorted(files) for tag, files in sorted(owed.items())}


def print_owed(owed: dict[str, list[str]]) -> None:
    total = sum(len(files) for files in owed.values())
    print(f"{total} manifest-managed capture(s) owed by the generator")
    for tag, files in owed.items():
        print(f"  {tag} ({len(files)}): {', '.join(files)}")


def unchecked_captures(
    manifest: dict, data: dict, source_dir: Path, checked: set[str]
) -> dict[str, list[str]]:
    """Manifest rows this pass verified nothing about, by why.

    A clean run says "N captures checked, 0 error(s)" and reads as a verdict on
    the gallery. It is not: this pass compares against the fixture's own specs,
    and two disjoint sets sit outside that without anything saying so.

    - `owed` -- no capture in the generator produces them. Reported already,
      and not a regression (task 39's remaining work).
    - `produced-unchecked` -- rendered from a filename hardcoded in the
      generator's C++ rather than named in screenshot_data.json. The generator
      writes them, and the existence check below covers that, but there is no
      spec to compare dimensions against.
    """
    rendered_by_fixture = {
        spec["filename"]
        for key in ("screenshots", "dialogs")
        for spec in data.get(key, [])
    }
    hardcoded = hardcoded_capture_filenames(source_dir)

    out: dict[str, list[str]] = {}
    for entry in manifest["images"]:
        tag = entry["tag"]
        name = entry["file"]
        if not tag.startswith("auto-"):
            continue
        if name in checked:
            continue
        if name in rendered_by_fixture or name not in hardcoded:
            # Owed, or a fixture spec that failed to appear -- both already
            # reported, as the owed set and as an error respectively.
            continue
        out.setdefault("produced-unchecked", []).append(name)
    return {kind: sorted(files) for kind, files in sorted(out.items())}


def print_unchecked(unchecked: dict[str, list[str]]) -> None:
    """Says what a clean run is *not* evidence about."""
    explanation = {
        "produced-unchecked": (
            "produced from a filename hardcoded in the generator's C++, so "
            "their existence is checked but no fixture spec exists to check "
            "their dimensions against"
        ),
    }
    for kind, files in unchecked.items():
        print(f"  {kind} ({len(files)}) -- {explanation[kind]}:")
        print(f"    {', '.join(files)}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    here = Path(__file__).resolve().parent
    parser.add_argument(
        "--generator",
        type=Path,
        help="client_screenshot_generator binary (not needed with "
        "--report-owed)",
    )
    parser.add_argument(
        "--data", type=Path, default=here / "screenshot_data.json"
    )
    parser.add_argument(
        "--image-manifest",
        type=Path,
        default=here / ".." / ".." / "screenshots" / "image_manifest.json",
    )
    parser.add_argument(
        "--report-owed",
        action="store_true",
        help="Print the owed set and exit, without running the generator",
    )
    parser.add_argument(
        "--out",
        type=Path,
        default=None,
        help="Render directory (default: a fresh temp dir)",
    )
    parser.add_argument(
        "--theme",
        default=None,
        help="Render under this appearance (dark|light|hc) instead of the "
        "generator's default.",
    )
    args = parser.parse_args()

    data = json.loads(args.data.read_text(encoding="utf-8"))
    manifest = json.loads(args.image_manifest.read_text(encoding="utf-8"))
    owed = owed_captures(manifest, data, Path(__file__).resolve().parent)

    if args.report_owed:
        print_owed(owed)
        return 0

    if args.generator is None:
        parser.error("--generator is required unless --report-owed is given")

    out_dir = args.out or Path(tempfile.mkdtemp(prefix="screenshot-check-"))
    out_dir.mkdir(parents=True, exist_ok=True)

    env = dict(os.environ)
    env["QT_QPA_PLATFORM"] = "offscreen"

    # One generator run per appearance the gallery holds. A single run can only
    # ever produce one of them -- the theme decides what the generator writes,
    # so checking the light rows against a dark run reports every one of them
    # as "not produced", which is what this did before the gallery grew a
    # second appearance.
    themes = [args.theme] if args.theme else gallery_themes(manifest)

    for theme in themes:
        command = [
            str(args.generator),
            f"--out={out_dir}",
            f"--image-manifest={args.image_manifest}",
            f"--data={args.data}",
            f"--theme={theme}",
        ]

        try:
            result = subprocess.run(
                command,
                env=env,
                cwd=args.generator.parent,
                timeout=240,
            )
        except subprocess.TimeoutExpired:
            print(
                f"error: generator timed out after 240s rendering {theme}",
                file=sys.stderr,
            )
            return 1
        if result.returncode != 0:
            print(
                f"error: generator exited with {result.returncode} "
                f"rendering {theme}",
                file=sys.stderr,
            )
            return 1

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
    checked_names: set[str] = set()
    for spec, exact_dims in specs:
        base_filename = spec["filename"]
        for theme in themes:
            filename = themed_name(base_filename, theme)
            if filename not in managed:
                continue
            path = out_dir / filename
            if not path.is_file():
                errors.append(f"{filename}: not produced")
                continue
            checked_names.add(filename)
            produced.append(path)
            width = spec.get("width")
            height = spec.get("height")
            if exact_dims and width and height:
                actual = png_dimensions(path)
                if actual != (width, height):
                    # The spec's dimensions are the theme-independent part:
                    # SaveScreenshot hard-resizes the widget, so a themed
                    # render that differs in SIZE is a defect rather than an
                    # appearance difference.
                    errors.append(
                        f"{filename}: {actual[0]}x{actual[1]}, spec says "
                        f"{width}x{height}"
                    )

    # A managed row whose filename lives in the C++ rather than in the fixture
    # is checked for existence, which is the whole point for those: several are
    # standalone panel captures with no spec at all, and three of them were once
    # rendered by nothing, which no dimension check could ever have said. Rows
    # nothing produces are the owed set, reported rather than failed on.
    hardcoded_names = hardcoded_capture_filenames(
        Path(__file__).resolve().parent
    )
    for filename in sorted((managed & hardcoded_names) - checked_names):
        path = out_dir / filename
        if not path.is_file():
            errors.append(f"{filename}: not produced")
            continue
        checked_names.add(filename)
        produced.append(path)

    for group in duplicate_renders(produced):
        names = ", ".join(sorted(p.name for p in group))
        errors.append(
            f"identical bytes, so one of them renders the other's picture: "
            f"{names}"
        )

    for error in errors:
        print(f"error: {error}", file=sys.stderr)

    # The denominator is the point: "39 captures checked, 0 error(s)" reads as
    # a verdict on the gallery, and this pass covers well under half of it.
    checked = len(checked_names)
    print(
        f"{checked} of {len(managed)} manifest-managed captures checked in "
        f"{out_dir}, {len(errors)} error(s)"
    )
    unchecked = unchecked_captures(
        manifest, data, Path(__file__).resolve().parent, checked_names
    )
    if unchecked or owed:
        print("what this run is not evidence about:")
    # Reported, never failed on: the owed set is the remaining work of task 39,
    # so a non-zero count is the backlog rather than a regression.
    print_unchecked(unchecked)
    print_owed(owed)
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
