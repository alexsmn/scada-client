#!/usr/bin/env python3
"""Capture provenance for the tracked screenshot gallery (task 434).

A screenshot is a claim about the tree that produced it, and nothing in the
pipeline checked that the claim still holds. `check_screenshots.py` asserts
properties of the *file* (it exists, its dimensions match the spec, no two
captures are byte-identical) and `validate_image_manifest.py` asserts the
manifest agrees with the manual. Neither can tell you that a tracked PNG was
rendered from a tree that no longer exists — so images drift while the gallery
still reads as a baseline, silently.

Re-rendering and comparing is the only thing that can *prove* freshness, and it
is expensive: it needs Qt and a build. It no longer needs Windows — that
requirement was retired on 2026-09-12 and survives only for the `manual-modus`
ActiveX family, which this script's `generated_rows` never covers. This script
is the cheap half:
it records, per generated image, the commit and platform it was captured at,
so staleness becomes **visible** without a re-render.

    captured: {commit, platform, dirty, sha256}

`sha256` is what makes the record self-verifying, and it is why this works at
all in a repository where nobody can be trusted to re-stamp by hand: if the
bytes on disk no longer match the digest, the recorded commit describes some
*other* image and the provenance is stale, whoever wrote it. That also solves
the "which files did this pass actually write" problem without the pass having
to say — a render that changed nothing leaves its provenance untouched, which
is correct, because the older commit is still the one that produced those
bytes.

`dirty` matters more than it looks. A capture taken from a modified worktree is
reproducible from *no* commit, so its `commit` is not a claim anyone can check.
Recording it is the difference between "stale" and "never verifiable".

Two modes:

    capture_provenance.py --stamp   # after a regeneration; rewrites changed rows
    capture_provenance.py --report  # what the gallery's provenance says today

Nothing here backfills. An image with no `captured` block reads as *unknown*,
which is the honest state of every image tracked before this existed — writing
a plausible commit for one would manufacture exactly the false baseline the
entry is about.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
import sys
from pathlib import Path

# Tracked paths whose content can change what a capture renders. Used only to
# decide the `dirty` flag: this repository is a shared checkout in which
# several sessions hold unrelated edits at once, so "the worktree is dirty" is
# always true and says nothing. Dirt *under these paths* is what makes a render
# unreproducible.
RENDER_PATHS = ("client", "common", "core")


def is_generator_owned(tag: str) -> bool:
    """Whether the generator produces this image, so it has provenance to record.

    Kept in step with validate_image_manifest.py's predicate of the same name: a
    hand-captured image has no capture commit to record, so stamping one would
    invent provenance rather than record it. `auto-*` is the whole set — a
    second tag, `reshell-theme`, sat beside it for the captures that existed
    only under the opt-in design-token theme, and went with that opt-in on
    2026-08-31.
    """
    return tag.startswith("auto-")


def platform_name() -> str:
    """The capture platform, at the granularity that changes the pixels.

    Offscreen Qt renders with the host's fonts, so a macOS render of an
    unchanged UI differs from a Windows one. That makes the platform part of
    the provenance rather than trivia: it is what separates platform churn from
    a real UI regression when a diff is read.
    """
    if sys.platform.startswith("win"):
        return "windows"
    if sys.platform == "darwin":
        return "macos"
    if sys.platform.startswith("linux"):
        return "linux"
    return sys.platform


def git(repo_root: Path, *args: str) -> str:
    return subprocess.run(
        ("git", *args),
        cwd=repo_root,
        check=True,
        capture_output=True,
        text=True,
    ).stdout.strip()


def head_commit(repo_root: Path) -> str:
    return git(repo_root, "rev-parse", "--short", "HEAD")


def render_paths_dirty(repo_root: Path) -> bool:
    """Whether any tracked file that can affect a render differs from HEAD."""
    existing = [p for p in RENDER_PATHS if (repo_root / p).exists()]
    if not existing:
        return False
    return bool(git(repo_root, "status", "--porcelain", "--", *existing))


def commit_is_in_history(repo_root: Path, commit: str) -> bool:
    """Whether `commit` is an ancestor of HEAD (or HEAD itself).

    A capture stamped on a branch that was never merged, or on a commit that a
    rebase has since rewritten, names a tree this history does not contain —
    which is precisely the neither-side-correct case that motivated the entry.
    """
    try:
        subprocess.run(
            ("git", "merge-base", "--is-ancestor", commit, "HEAD"),
            cwd=repo_root,
            check=True,
            capture_output=True,
        )
        return True
    except subprocess.CalledProcessError:
        return False


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def generated_rows(manifest: dict) -> list[dict]:
    return [row for row in manifest["images"] if is_generator_owned(row.get("tag", ""))]


def stamp(manifest: dict, images_dir: Path, provenance: dict) -> list[str]:
    """Record `provenance` on every generated image whose bytes changed.

    Returns the filenames restamped. A row whose digest still matches keeps the
    provenance it had: those bytes were produced by that older commit, and
    overwriting it with today's would claim a freshness the render did not
    establish.
    """
    restamped = []
    for row in generated_rows(manifest):
        image = images_dir / row["file"]
        if not image.exists():
            continue
        current = digest(image)
        if row.get("captured", {}).get("sha256") == current:
            continue
        row["captured"] = {**provenance, "sha256": current}
        restamped.append(row["file"])
    return restamped


def report(manifest: dict, images_dir: Path, repo_root: Path | None) -> list[str]:
    """One line per generated image whose provenance is absent or not credible.

    Silence means every tracked generated image records a clean capture, from a
    commit in this history, whose bytes are still the ones that were stamped.
    """
    lines = []
    for row in sorted(generated_rows(manifest), key=lambda r: r["file"]):
        name = row["file"]
        image = images_dir / name
        if not image.exists():
            continue
        captured = row.get("captured")
        if not captured:
            lines.append(f"{name}: provenance unknown - never stamped")
            continue
        if captured.get("sha256") != digest(image):
            lines.append(
                f"{name}: provenance STALE - the file has changed since it was "
                f"stamped at {captured.get('commit', '?')}"
            )
            continue
        if captured.get("dirty"):
            lines.append(
                f"{name}: captured from a dirty tree at {captured.get('commit', '?')} "
                f"- reproducible from no commit"
            )
            continue
        commit = captured.get("commit")
        if repo_root and commit and not commit_is_in_history(repo_root, commit):
            lines.append(
                f"{name}: captured at {commit}, which is not in this history "
                f"- rebased away, or never merged"
            )
    return lines


def platform_summary(manifest: dict, images_dir: Path) -> list[str]:
    """Which platforms the tracked gallery was rendered on, and how much of it.

    CLAUDE.md records that the Qt gallery is not a verified Windows baseline —
    at least twelve tracked images are macOS renders — so the first Windows
    regeneration will rewrite an unknown number of them as platform churn that
    nobody can distinguish from a regression. This turns "an unknown number"
    into a count.
    """
    counts: dict[str, int] = {}
    for row in generated_rows(manifest):
        if not (images_dir / row["file"]).exists():
            continue
        platform = (row.get("captured") or {}).get("platform", "unknown")
        counts[platform] = counts.get(platform, 0) + 1
    return [f"  {platform}: {count}" for platform, count in sorted(counts.items())]


def main(argv: list[str] | None = None) -> int:
    here = Path(__file__).resolve()
    default_manifest = here.parents[2] / "screenshots" / "image_manifest.json"

    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--stamp", action="store_true",
                      help="record provenance on generated images whose bytes changed")
    mode.add_argument("--report", action="store_true",
                      help="print generated images whose provenance is absent or stale")
    parser.add_argument("--image-manifest", type=Path, default=default_manifest)
    parser.add_argument("--images-dir", type=Path, default=None,
                        help="defaults to the manifest's own directory")
    parser.add_argument("--repo-root", type=Path, default=None,
                        help="defaults to the manifest's repository")
    args = parser.parse_args(argv)

    manifest_path = args.image_manifest.resolve()
    images_dir = args.images_dir or manifest_path.parent
    repo_root = args.repo_root
    if repo_root is None:
        try:
            repo_root = Path(git(manifest_path.parent, "rev-parse", "--show-toplevel"))
        except (subprocess.CalledProcessError, OSError):
            repo_root = None

    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))

    if args.stamp:
        if repo_root is None:
            print("capture_provenance: not a git repository; nothing stamped",
                  file=sys.stderr)
            return 1
        provenance = {
            "commit": head_commit(repo_root),
            "platform": platform_name(),
            "dirty": render_paths_dirty(repo_root),
        }
        restamped = stamp(manifest, images_dir, provenance)
        if restamped:
            manifest_path.write_text(
                json.dumps(manifest, ensure_ascii=False, indent=2) + "\n",
                encoding="utf-8",
            )
        print(f"capture provenance: {len(restamped)} image(s) restamped at "
              f"{provenance['commit']} on {provenance['platform']}"
              + (" (DIRTY TREE)" if provenance["dirty"] else ""))
        for name in restamped:
            print(f"  {name}")
        return 0

    lines = report(manifest, images_dir, repo_root)
    print("capture provenance by platform:")
    for line in platform_summary(manifest, images_dir):
        print(line)
    if lines:
        # Reported, never failed on: every image tracked before this existed is
        # legitimately unknown, so a non-zero count is the backlog rather than a
        # regression. Same rule as check_screenshots.py's owed set.
        print(f"{len(lines)} image(s) with absent or unverifiable provenance:")
        for line in lines:
            print(f"  {line}")
    else:
        print("every tracked generated image records a verifiable capture")
    return 0


if __name__ == "__main__":
    sys.exit(main())
