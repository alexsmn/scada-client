#!/usr/bin/env python3
"""Consistency check between image_manifest.json and the user manual.

The manual is the `scada-docs/` tree of this repository (it was a sibling
repository until the 2026-08-08 graft). The manifest
(client/screenshots/image_manifest.json) is the source of truth for every image
the manual ships. This script verifies:

  1. Bijection: every file in scada-docs img/ has a manifest entry, and every
     manifest entry not marked "published": false exists in img/.
  2. referenced_from matches the actual references from the Russian
     (canonical) manual pages. English mirrors under en/ are implied by
     _data/i18n_pages.yml and deliberately not listed in the manifest.
  3. The per-tag "counts" block matches the entries.
  4. "obsolete" entries are referenced by no page at all (RU or EN).
  5. current_generator_owned_subset entries exist and are generator-owned
     (an auto-* tag).
  6. No entry carries publish_theme. It named which of two render passes
     produced a published image, and there is only one pass now.

Run it after changing the manifest, the images, or any manual page:

    python3 client/screenshots/validate_image_manifest.py

Exit code 0 = consistent, 1 = violations found (each printed on stderr).
"""

import argparse
import collections
import json
import re
import sys
from pathlib import Path

IMAGE_REF_RE = re.compile(r"img/([\w.\-]+\.(?:png|jpe?g|gif|svg))")

def is_generator_owned(tag: str) -> bool:
    """Whether the generator produces this image, and it may be published.

    `auto-*` is the whole set. A second tag, `reshell-theme`, used to sit
    beside it for captures that existed only under the opt-in design-token
    theme — hardware-tree.png's status dots, the config-workbench subtabs, the
    panels the shell built only when the theme was on. There is no opt-in any
    more, so those are ordinary `auto-view` / `auto-dialog` rows.
    """
    return tag.startswith("auto-")


# Non-content markdown that may mention image names without embedding them.
EXCLUDED_PAGES = {"CLAUDE.md", "README.md", "tasks.md"}

def find_default_docs_repo(manifest_path: Path) -> Path | None:
    """Locate the manual: scada-docs/ in this repository, and nowhere else.

    Deliberately does NOT fall back to a `../scada-docs` sibling. The manual was
    a separate repository until 2026-08-08, and a frozen pre-graft checkout still
    sits beside the monorepo on the machine where it was developed; validating
    against that copy would pass while the tree that actually ships drifted. Two
    trees named scada-docs is precisely the case worth failing on.

    The manifest lives at <root>/client/screenshots/image_manifest.json, so the
    repository root is parents[2]. It was parents[3] until this was corrected —
    right when the manifest was one directory deeper, one above the root after it
    moved, and harmless only for as long as the sibling it then resolved to was
    the live manual.
    """
    root = manifest_path.resolve().parents[2]
    candidate = root / "scada-docs"
    return candidate if (candidate / "img").is_dir() else None


def collect_references(docs_repo: Path) -> dict[str, set[str]]:
    """Map image filename -> set of docs-root-relative pages referencing it."""
    refs: dict[str, set[str]] = collections.defaultdict(set)
    for page in docs_repo.rglob("*.md"):
        rel = page.relative_to(docs_repo)
        if rel.parts and rel.parts[0].startswith("_"):
            continue
        if rel.name in EXCLUDED_PAGES:
            continue
        text = page.read_text(encoding="utf-8", errors="replace")
        for match in IMAGE_REF_RE.finditer(text):
            refs[match.group(1)].add(rel.as_posix())
    return refs


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--manifest",
        type=Path,
        default=Path(__file__).resolve().parent / "image_manifest.json",
    )
    parser.add_argument(
        "--docs-repo",
        type=Path,
        default=None,
        help="Path to the manual tree (default: scada-docs/ in this repository)",
    )
    args = parser.parse_args()

    docs_repo = args.docs_repo or find_default_docs_repo(args.manifest)
    if docs_repo is None or not (docs_repo / "img").is_dir():
        print(
            "error: scada-docs checkout not found; pass --docs-repo",
            file=sys.stderr,
        )
        return 1

    manifest = json.loads(args.manifest.read_text(encoding="utf-8"))
    entries = {e["file"]: e for e in manifest["images"]}
    errors: list[str] = []

    img_files = {
        p.name for p in (docs_repo / "img").iterdir() if p.is_file()
    }

    # 1. Bijection img/ <-> manifest.
    for name in sorted(img_files - entries.keys()):
        errors.append(f"img/{name} has no manifest entry")
    for name, entry in sorted(entries.items()):
        if entry.get("published") is False:
            if name in img_files:
                errors.append(
                    f"{name} is marked published:false but exists in img/"
                )
            continue
        if name not in img_files:
            errors.append(f"manifest entry {name} is missing from img/")

    # 2. referenced_from vs actual Russian-page references.
    all_refs = collect_references(docs_repo)
    for name, entry in sorted(entries.items()):
        actual_ru = sorted(
            r for r in all_refs.get(name, set()) if not r.startswith("en/")
        )
        stated = sorted(entry.get("referenced_from", []))
        if entry.get("published") is False:
            if actual_ru:
                errors.append(
                    f"{name} is published:false but referenced by {actual_ru}"
                )
            continue
        if stated != actual_ru:
            errors.append(
                f"{name}: referenced_from {stated} != actual RU references "
                f"{actual_ru}"
            )

    # 3. Tag counts.
    actual_counts = collections.Counter(e["tag"] for e in manifest["images"])
    stated_counts = manifest.get("counts", {})
    if dict(actual_counts) != stated_counts:
        errors.append(
            f"counts block {stated_counts} != actual {dict(actual_counts)}"
        )

    # 4. Obsolete entries must be fully unreferenced (RU and EN).
    for name, entry in sorted(entries.items()):
        if entry["tag"] == "obsolete" and all_refs.get(name):
            errors.append(
                f"{name} is obsolete but referenced by "
                f"{sorted(all_refs[name])}"
            )

    # 5. Publish subset sanity.
    published_subset = manifest.get("current_generator_owned_subset", [])
    for name in published_subset:
        entry = entries.get(name)
        if entry is None:
            errors.append(f"publish subset entry {name} not in manifest")
        elif not is_generator_owned(entry["tag"]):
            errors.append(
                f"publish subset entry {name} has non-generated tag "
                f"{entry['tag']}"
            )

    # 6. publish_theme is retired. It chose between two render passes — a
    #    default pass that rendered the un-themed "Classic" client and a second
    #    `--theme=dark` pass over the published subset — and named the opt-out
    #    for an image that could not render themed. There is one pass now, so
    #    the field reads as policy and changes nothing, which is worse than
    #    absent.
    for name, entry in sorted(entries.items()):
        if "publish_theme" in entry:
            errors.append(
                f"{name}: publish_theme is retired — there is one render pass "
                f"and one appearance; delete the field"
            )

    for error in errors:
        print(f"error: {error}", file=sys.stderr)
    print(
        f"{len(entries)} manifest entries, {len(img_files)} files in img/, "
        f"{len(errors)} error(s)"
    )
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
