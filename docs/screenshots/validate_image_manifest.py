#!/usr/bin/env python3
"""Consistency check between image_manifest.json and the scada-docs repo.

The manifest (client/docs/screenshots/image_manifest.json) is the source of
truth for every image the web manual ships. This script verifies:

  1. Bijection: every file in scada-docs img/ has a manifest entry, and every
     manifest entry not marked "published": false exists in img/.
  2. referenced_from matches the actual references from the Russian
     (canonical) manual pages. English mirrors under en/ are implied by
     _data/i18n_pages.yml and deliberately not listed in the manifest.
  3. The per-tag "counts" block matches the entries.
  4. "obsolete" entries are referenced by no page at all (RU or EN).
  5. current_generator_owned_subset entries exist and carry an auto-* tag.

Run it after changing the manifest, the images, or any manual page:

    python3 client/docs/screenshots/validate_image_manifest.py \
        --docs-repo ../scada-docs

Exit code 0 = consistent, 1 = violations found (each printed on stderr).
"""

import argparse
import collections
import json
import re
import sys
from pathlib import Path

IMAGE_REF_RE = re.compile(r"img/([\w.\-]+\.(?:png|jpe?g|gif|svg))")

# Non-content markdown that may mention image names without embedding them.
EXCLUDED_PAGES = {"CLAUDE.md", "README.md", "tasks.md"}


def find_default_docs_repo(manifest_path: Path) -> Path | None:
    scada_root = manifest_path.resolve().parents[3]
    for candidate in (scada_root / "scada-docs", scada_root.parent / "scada-docs"):
        if (candidate / "img").is_dir():
            return candidate
    return None


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
        help="Path to the scada-docs checkout (default: <scada>/scada-docs "
        "or a sibling of the scada repo)",
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
    for name in manifest.get("current_generator_owned_subset", []):
        entry = entries.get(name)
        if entry is None:
            errors.append(f"publish subset entry {name} not in manifest")
        elif not entry["tag"].startswith("auto-"):
            errors.append(
                f"publish subset entry {name} has non-auto tag {entry['tag']}"
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
