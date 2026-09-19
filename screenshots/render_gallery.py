#!/usr/bin/env python3
"""Render screenshots/README.md — the public gallery — from image_manifest.json.

The gallery is what a GitHub visitor sees when they open `screenshots/`: every
generated capture, both appearances, with a link to the manual page that
documents the surface. It is generated rather than hand-written because the
capture set moves with the UI, and a hand-written index goes stale silently —
the tree has four hand-repairs of exactly that failure on another generated
page (see the superproject CLAUDE.md, "Recording a finding stales the sheet").

    python3 screenshots/render_gallery.py            # CHECK: exit 1 if stale
    python3 screenshots/render_gallery.py --render   # write screenshots/README.md

Bare invocation is the check, matching `render_capture_review.py` in the
superproject. It is registered as the ctest `client_screenshot_gallery_check`
(client/tools/CMakeLists.txt) and is source-only: no build, no Qt, no network.

WHAT IT PUBLISHES, AND WHAT IT DOES NOT
---------------------------------------
Only `auto-*` rows whose PNG is present in this directory — the generator-owned
set, 72 dark/light pairs. `manual-*` rows are hand-maintained captures (the five
Modus ones need a Windows COM control) and `obsolete` rows document retired UI;
neither belongs in a gallery that claims to show the current client.

It publishes the filename and the manual link, and deliberately NOT the
manifest's `notes`. Those are written for whoever maintains the capture —
fixture seeding, backlog numbers, what a diff against the row would mean — and
amplifying them onto a landing page would read as internal noise to the
audience this page is for. The manifest remains the place to read them.

PAIRING LIGHT WITH DARK
-----------------------
A capture's light render is `<base>-light.png`, but the FILENAME is not what
decides it: the superproject CLAUDE.md records that reading a theme off a
filename misread `devices-create.png` — a separate capture — as a themed
variant of `devices.png`. So the name only proposes a candidate, and the pair
is admitted only when the manifest's own `theme` field says one row is `light`
and the other is not. A row with no `theme` is never paired.

THE MANUAL LINK
---------------
Derived from the row's `referenced_from`, which names a Russian manual page
(`client/graph.md`), as `https://telecontrol-ru.github.io/scada/en/<stem>/`.
That mapping is the Jekyll `permalink` convention in scada-docs, and it is a
convention rather than something this repo can verify: the client repo is
standalone (it must never reference the superproject or a sibling checkout), so
the manual is not on disk here. All 16 pages the manifest references were
confirmed to resolve on 2026-09-19; a manual reorganisation would break them
silently, which is the accepted cost of not coupling the repos.
"""

import argparse
import json
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
MANIFEST = HERE / "image_manifest.json"
GALLERY = HERE / "README.md"

MANUAL_BASE = "https://telecontrol-ru.github.io/scada/en"

# Section order and headings, keyed by manifest tag.
SECTIONS = [
    ("auto-view", "Views", "Workspace views and panels — what the operator works in."),
    ("auto-dialog", "Dialogs", "Modal and overlay surfaces."),
    ("auto-menu", "Menus", "Menus and context menus, captured open."),
]

PREVIEW_WIDTH = 360


def manual_url(row: dict) -> str | None:
    """The English manual page for a capture, or None if it documents no page."""
    refs = row.get("referenced_from") or []
    if not refs:
        return None
    return f"{MANUAL_BASE}/{refs[0].removesuffix('.md')}/"


def title_of(file_name: str) -> str:
    """A human title from the filename — the manifest states no display name."""
    stem = file_name.removesuffix(".png").replace("-", " ")
    return stem[:1].upper() + stem[1:]


def collect(manifest: dict) -> dict[str, list[tuple[dict, dict | None]]]:
    """Group present `auto-*` captures by tag, each paired with its light render.

    Returns {tag: [(primary_row, light_row_or_None), ...]}, each list sorted by
    filename so the page's order is stable across runs.
    """
    rows = {r["file"]: r for r in manifest["images"]}
    present = {name for name, r in rows.items() if (HERE / name).exists()}

    def is_light(name: str) -> bool:
        return rows.get(name, {}).get("theme") == "light"

    grouped: dict[str, list[tuple[dict, dict | None]]] = {tag: [] for tag, _, _ in SECTIONS}
    for name in sorted(present):
        row = rows[name]
        tag = row.get("tag", "")
        if tag not in grouped or is_light(name):
            continue
        candidate = name.removesuffix(".png") + "-light.png"
        # The declared theme decides, never the filename. See the module docstring.
        light = rows[candidate] if candidate in present and is_light(candidate) else None
        grouped[tag].append((row, light))
    return grouped


def render(manifest: dict) -> str:
    grouped = collect(manifest)
    total = sum(len(v) for v in grouped.values())

    out: list[str] = []
    out.append("# Screen gallery")
    out.append("")
    out.append(
        f"{total} captures of the Telecontrol SCADA client, rendered offscreen from a "
        "fixture by [`client_screenshot_generator`](../tools/screenshot_generator) and "
        "tracked in this repository — so a UI change lands as a reviewable image diff "
        "rather than being noticed the next time somebody refreshes the manual."
    )
    out.append("")
    out.append(
        "Each capture is rendered in both appearances. The client follows the host "
        "OS light/dark preference by default, so both are what operators actually see. "
        "The previews below are the dark render; the light one is linked beside it."
    )
    out.append("")
    out.append(
        "> Generated by [`render_gallery.py`](render_gallery.py) from "
        "[`image_manifest.json`](image_manifest.json) — edit those, not this file."
    )
    out.append("")

    for tag, heading, blurb in SECTIONS:
        entries = grouped.get(tag) or []
        if not entries:
            continue
        out.append(f"## {heading}")
        out.append("")
        out.append(f"{blurb} ({len(entries)})")
        out.append("")
        out.append("| Capture | Preview | |")
        out.append("|---|---|---|")
        for row, light in entries:
            name = row["file"]
            links = []
            if light is not None:
                links.append(f"[Light]({light['file']})")
            url = manual_url(row)
            if url:
                links.append(f"[Manual]({url})")
            preview = f'<img src="{name}" alt="{title_of(name)}" width="{PREVIEW_WIDTH}">'
            out.append(f"| **{title_of(name)}**<br>`{name}` | {preview} | {'<br>'.join(links)} |")
        out.append("")

    out.append("## Reading a diff")
    out.append("")
    out.append(
        "Every row in the manifest records the commit and host platform it was "
        "captured on. Offscreen Qt rasterizes with the host's fonts, so **a changed "
        "image whose `platform` also changed is churn, and a changed image on the "
        "same platform is a UI change**."
    )
    out.append("")
    return "\n".join(out)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument(
        "--render",
        action="store_true",
        help="write screenshots/README.md instead of checking it",
    )
    args = parser.parse_args()

    manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
    rendered = render(manifest)

    if args.render:
        GALLERY.write_text(rendered, encoding="utf-8")
        print(f"wrote {GALLERY.relative_to(HERE.parent)}")
        return 0

    current = GALLERY.read_text(encoding="utf-8") if GALLERY.exists() else ""
    if current == rendered:
        print(f"{GALLERY.name} is current")
        return 0
    print(
        f"{GALLERY.name} is stale — the manifest or the capture set has moved since it "
        "was rendered.\nRun: python3 screenshots/render_gallery.py --render",
        file=sys.stderr,
    )
    return 1


if __name__ == "__main__":
    sys.exit(main())
