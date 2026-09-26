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
*other* image and the provenance is stale, whoever wrote it.

What the digest does **not** answer is which files a run rendered, and this
module claimed for a year that it did — the paragraph above used to end "that
also solves the *which files did this pass actually write* problem without the
pass having to say". It does not (tasks 642 and 816). "The bytes differ from
what is recorded" and "this run produced these bytes" coincide only once a row
has a record to differ from, and only when nothing else in the gallery moved:
a partial render followed by `--stamp`, in a checkout where a peer holds an
unrelated capture modified, stamps that peer's image with this run's commit.
That is an ordinary evening in this tree. The web mirror had the identical
shape and one `--only` run wrote its commit onto all 22 of its rows
(`b52ad74e9`, task 599).

So `--stamp` is **told** what the run rendered and never infers it. A caller
says so with `--produced`, `--produced-from` or `--produced-since`, and a
`--stamp` that says nothing fails rather than guessing: defaulting to
"everything" restores the backfill and defaulting to "nothing" would quietly
stop recording provenance at all. Within that list the digest still governs —
a rendered image whose bytes came out identical keeps the older commit that
genuinely produced them, which is correct and keeps a no-op run from churning
the manifest.

`dirty` matters more than it looks. A capture taken from a modified worktree is
reproducible from *no* commit, so its `commit` is not a claim anyone can check.
Recording it is the difference between "stale" and "never verifiable".

Three modes:

    capture_provenance.py --stamp --produced-since <marker>   # after a render
    capture_provenance.py --report   # what the gallery's provenance says today
    capture_provenance.py --check-pairs [--baseline PATH]     # task 785

`--check-pairs` answers the question the per-image record cannot: every
generated capture is rendered twice, dark and light, in two separate passes,
and a fix re-rendered in one appearance leaves its sibling documenting the old
UI while both records stay self-consistent. It pairs the rows by their `theme`
field and reports a capture whose sibling changed after it was last rendered
with something under RENDER_PATHS moving in between. That needs to know when
an image was last *rendered*, not only when its bytes last changed, so
`--stamp` also records `captured.last_rendered` on every row the run rendered.

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
from collections.abc import Iterable
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


def render_paths_dirty(repo_root: Path, images_dir: Path | None = None) -> bool:
    """Whether any tracked file that can affect a render differs from HEAD.

    The gallery itself is excluded, and without that exclusion this function
    cannot return False during the one operation that calls it. The images live
    under `client/`, which is a render path, so a regeneration dirties the very
    tree it is asking about: every one of the 150 stamped rows read
    `"dirty": true` when this was measured (2026-09-20), including rows stamped
    from an otherwise pristine checkout. A flag that is always set carries no
    information, and this one was worse than useless -- `report()` returns on
    it, so the "captured at a commit that is not in this history" check below
    never ran for any image.

    What a render consumes is the source; what it produces is the gallery. Only
    the first can make a capture unreproducible.
    """
    existing = [p for p in RENDER_PATHS if (repo_root / p).exists()]
    if not existing:
        return False
    pathspec = list(existing)
    if images_dir is not None:
        try:
            rel = images_dir.resolve().relative_to(repo_root.resolve())
        except ValueError:
            rel = None
        if rel is not None:
            pathspec.append(f":(exclude){rel.as_posix()}")
    return bool(git(repo_root, "status", "--porcelain", "--", *pathspec))


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


def stamp(manifest: dict, images_dir: Path, provenance: dict,
          produced: Iterable[str] | None) -> list[str]:
    """Record `provenance` on the images in `produced` whose bytes changed.

    `produced` is the list of filenames this run rendered, and it is required:
    the two questions "did the bytes move" and "did this run write them" are
    different, and answering the second with the first is tasks 642 and 816.
    A row outside `produced` is never touched whatever its digest says.

    Returns the filenames restamped. Two cases inside `produced` are easy to
    lose and are both deliberate. A rendered row whose digest still matches
    keeps the provenance it had — that older commit did produce those bytes,
    and overwriting it would claim a freshness this render did not establish.
    A rendered row with no record at all *does* get its first one, which is not
    a backfill: this run is what made those bytes.

    Raises `TypeError` when `produced` is None. Loud rather than silent in
    either direction — defaulting to "everything" restores the backfill this
    gate exists to stop, and defaulting to "nothing" would quietly stop
    recording provenance while still reporting success.
    """
    if produced is None:
        raise TypeError(
            "stamp() needs the list of files this run rendered "
            "(tasks 642 and 816)")
    rendered = produced if isinstance(produced, (set, frozenset)) else set(produced)
    restamped = []
    for row in generated_rows(manifest):
        if row["file"] not in rendered:
            continue
        image = images_dir / row["file"]
        if not image.exists():
            continue
        current = digest(image)
        if row.get("captured", {}).get("sha256") == current:
            continue
        row["captured"] = {**provenance, "sha256": current}
        restamped.append(row["file"])
    return restamped


def record_renders(manifest: dict, images_dir: Path, commit: str,
                   produced: Iterable[str]) -> list[str]:
    """Record `commit` as the last render of every row in `produced`.

    This is the half `stamp()` deliberately does not answer. `captured.commit`
    is the commit that last CHANGED an image's bytes -- a re-render that comes
    out identical keeps the older commit, correctly, because that commit did
    produce those bytes. So it says nothing about when the image was last
    looked at, and "was the light sibling rendered after the dark one changed?"
    is exactly that question (task 785). `captured.last_rendered` records it:
    every row this run rendered gets it, whether or not its bytes moved.

    Only rows that already carry a `captured` record are touched -- call this
    after `stamp()`, which gives a first record to every rendered row that had
    none. Returns the filenames whose `last_rendered` changed, so a pass that
    renders nothing new rewrites nothing.
    """
    rendered = produced if isinstance(produced, (set, frozenset)) else set(produced)
    touched = []
    for row in generated_rows(manifest):
        if row["file"] not in rendered or not (images_dir / row["file"]).exists():
            continue
        captured = row.get("captured")
        if not captured or captured.get("last_rendered") == commit:
            continue
        captured["last_rendered"] = commit
        touched.append(row["file"])
    return touched


def theme_pairs(manifest: dict) -> tuple[list[tuple[dict, dict]], list[str]]:
    """Each generated capture paired with its light sibling, plus the orphans.

    A capture's light render is `<base>-light.png`, but the filename only
    PROPOSES the pair: it is admitted when the manifest's own `theme` field says
    one row is `light` and the other is not. Reading the theme off the name is
    the mistake that took `devices-create.png` -- a separate capture -- for a
    themed variant of `devices.png` (root CLAUDE.md; `render_gallery.py` pairs
    the same way). Rows that declare no theme are never paired and never
    orphans: they are the owed captures nothing renders yet.

    Returns (pairs, findings). A finding is a themed row with no sibling in the
    other appearance -- the publish-time gap task 785 named: three dialog
    captures got a light sibling on 2026-09-19 only because the rows being
    copied happened to have one.
    """
    rows = {row["file"]: row for row in generated_rows(manifest)}
    pairs = []
    findings = []
    for name, row in sorted(rows.items()):
        theme = row.get("theme")
        if not theme:
            continue
        if theme == "light":
            base = name.removesuffix("-light.png") + ".png"
            base_row = rows.get(base)
            if not name.endswith("-light.png") or base_row is None \
                    or base_row.get("theme") in (None, "light"):
                findings.append(f"{name}: light render with no themed base capture")
            continue
        sibling = rows.get(name.removesuffix(".png") + "-light.png")
        if sibling is None or sibling.get("theme") != "light":
            findings.append(f"{name}: {theme} capture with no light sibling")
            continue
        pairs.append((row, sibling))
    return pairs, findings


def _merge_base(repo_root: Path, a: str, b: str) -> str | None:
    try:
        return git(repo_root, "merge-base", a, b) or None
    except subprocess.CalledProcessError:
        return None


def render_paths_changed(repo_root: Path, since: str, until: str,
                         images_dir: Path | None) -> list[str] | None:
    """Files under RENDER_PATHS that differ between `since` and `until`.

    The gallery itself is excluded, for the reason `render_paths_dirty` gives:
    the images and the manifest live under `client/`, so without the exclusion
    the commit that tracked the other appearance's PNGs would count as a change
    to what renders them. None when git cannot answer (an unknown commit).
    """
    pathspec = [p for p in RENDER_PATHS if (repo_root / p).exists()]
    if images_dir is not None:
        try:
            rel = images_dir.resolve().relative_to(repo_root.resolve())
            pathspec.append(f":(exclude){rel.as_posix()}")
        except ValueError:
            pass
    try:
        out = git(repo_root, "diff", "--name-only", f"{since}..{until}", "--",
                  *pathspec)
    except subprocess.CalledProcessError:
        return None
    return out.split() if out else []


def pair_findings(manifest: dict, repo_root: Path,
                  images_dir: Path | None = None) -> dict[str, str]:
    """Captures whose sibling in the other appearance changed after they were
    last rendered, keyed by the capture that is BEHIND (task 785).

    For a pair (A, B) and each direction: A's bytes last changed at
    `A.captured.commit`; B was last rendered at `B.captured.last_rendered`
    (falling back to `B.captured.commit` for a row recorded before that field
    existed -- the latest render the record can vouch for). B is behind when
    its last render is not on or after A's change AND something under
    RENDER_PATHS moved in between. If nothing moved, A's change came from no
    source change B could have missed, and B is not owed a render.

    The predicate is deliberately NOT "the two were captured at the same
    commit". The themes are always two passes, so that is false for nearly
    every pair; and `captured.commit` is when the bytes last changed rather than
    when they were last rendered, so comparing the two recorded commits directly
    flagged 60 of 75 pairs on the day this was written, every one a pair where
    an identical re-render had simply left no trace. That is what
    `last_rendered` is for.

    Unstamped rows and commits git cannot resolve are findings too: this check
    fails closed, and `--report` says which of those it is.
    """
    findings: dict[str, str] = {}
    diff_cache: dict[tuple[str, str], list[str] | None] = {}

    def behind(changed: dict, other: dict) -> str | None:
        changed_at = (changed.get("captured") or {}).get("commit")
        other_rec = other.get("captured") or {}
        seen_at = other_rec.get("last_rendered") or other_rec.get("commit")
        if not changed_at or not seen_at:
            return (f"cannot be compared with {changed['file']} - "
                    f"one of the pair has no recorded capture")
        if changed_at == seen_at:
            return None
        if commit_is_ancestor(repo_root, changed_at, seen_at):
            return None
        # The changed side's current bytes were ALSO produced at its own last
        # render. If the other side has been rendered on that tree or a later
        # one, it has seen whatever those bytes reflect, wherever
        # `captured.commit` points. This is what settles a pair stamped at a
        # commit that was rebased away: both sides re-rendered identically at
        # one commit are in step, though the commit their bytes first appeared
        # at is not in this history (device-watch, 2026-09-26).
        changed_seen = (changed.get("captured") or {}).get("last_rendered")
        if changed_seen and commit_is_ancestor(repo_root, changed_seen, seen_at):
            return None
        base = seen_at if commit_is_ancestor(repo_root, seen_at, changed_at) \
            else _merge_base(repo_root, seen_at, changed_at)
        if base is None:
            return (f"cannot be compared with {changed['file']} - git cannot "
                    f"relate {seen_at} and {changed_at}")
        key = (base, changed_at)
        if key not in diff_cache:
            diff_cache[key] = render_paths_changed(repo_root, base, changed_at,
                                                   images_dir)
        moved = diff_cache[key]
        if moved is None:
            return (f"cannot be compared with {changed['file']} - git cannot "
                    f"diff {base}..{changed_at}")
        if not moved:
            return None
        return (f"behind {changed['file']} - that changed at {changed_at}, "
                f"this was last rendered at {seen_at}, and {len(moved)} "
                f"render-path file(s) moved in between (e.g. {moved[0]})")

    pairs, _ = theme_pairs(manifest)
    for first, second in pairs:
        for changed, other in ((first, second), (second, first)):
            reason = behind(changed, other)
            if reason:
                findings[other["file"]] = reason
    return findings


def commit_is_ancestor(repo_root: Path, ancestor: str, descendant: str) -> bool:
    """Whether `ancestor` is `descendant` or one of its ancestors."""
    return subprocess.run(
        ("git", "merge-base", "--is-ancestor", ancestor, descendant),
        cwd=repo_root, capture_output=True).returncode == 0


def check_pairs(manifest: dict, repo_root: Path, images_dir: Path | None,
                baseline: dict | None) -> tuple[list[str], list[str]]:
    """The `--check-pairs` verdict: (failures, notes).

    Failures are orphans, pair findings the baseline does not carry, and
    baseline entries that no longer describe a finding -- the last makes the
    worklist self-cleaning, the shape of every other baseline in this tree, so
    closing a gap forces deleting its line. Notes are the findings the baseline
    does carry, printed so the worklist stays visible.
    """
    _, orphans = theme_pairs(manifest)
    findings = pair_findings(manifest, repo_root, images_dir)
    carried = set((baseline or {}).get("behind", {}))
    failures = list(orphans)
    notes = []
    for name, reason in sorted(findings.items()):
        (notes if name in carried else failures).append(f"{name}: {reason}")
    for name in sorted(carried - set(findings)):
        failures.append(
            f"{name}: in the baseline but no longer behind its sibling - "
            f"delete its entry")
    return failures, notes


def produced_since(images_dir: Path, manifest: dict, marker: Path) -> set[str]:
    """Generated images modified at or after `marker`'s mtime.

    The pipeline's answer to "what did this run render". A caller that can
    enumerate its output should say so with `--produced`, which is exact; this
    is for the render passes that cannot, which is all of them today — the
    generator writes its PNGs from scattered call sites and keeps no list, so
    the marker file `regenerate_client_screenshots` touches immediately before
    the pass is the only thing in the pipeline that knows when it started.

    It is a window rather than a list, so it inherits one failure the explicit
    form does not: a file some other session writes *during* the render is
    inside the window and reads as produced. That window is the render, where
    the old behaviour's window was all of history — and the digest gate still
    applies within it. Narrow it further by rendering from a worktree, which is
    what the shared checkout's own rules already ask for.

    `>=` rather than `>`: the marker is created immediately before the pass, and
    on a filesystem with coarse timestamps the first image can land in the same
    tick. Including the boundary risks nothing here, since the marker is not an
    image and the digest gate decides what actually gets written.
    """
    since = marker.stat().st_mtime
    return {
        row["file"]
        for row in generated_rows(manifest)
        if (images_dir / row["file"]).exists()
        and (images_dir / row["file"]).stat().st_mtime >= since
    }


def report(manifest: dict, images_dir: Path, repo_root: Path | None) -> list[str]:
    """One line per finding about a generated image's provenance.

    Silence means every tracked generated image records a clean capture, from a
    commit in this history, whose bytes are still the ones that were stamped.

    One image can raise more than one finding, and that is the point of the
    ordering below. `dirty` and "not in this history" are independent facts
    about different parts of the record — one is about the worktree the capture
    came from, the other about whether the commit it names still exists — so a
    dirty capture is *also* checked against history. It was not until task 815:
    this function returned on the flag, and because `render_paths_dirty()`
    examined `client` while the gallery it stamps lives under `client/`, every
    row stamped before `d59515aa9` reads `dirty: true` whatever the tree held.
    144 of the 150 stamped rows did, so the history check below had never run
    on a row that reached it and read as passing.

    The earlier two checks *do* stop, and for a reason the other two do not
    share: an unstamped row has no commit to ask about, and a row whose bytes
    no longer match the digest has a commit that describes some other image.
    Neither leaves a question the history check could answer.
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


def read_produced_list(path: Path) -> set[str]:
    """The filenames a render pass recorded, from a list file or a JSON array.

    Both shapes because the two producers differ: a shell pass writes lines,
    and anything porting the web tool's `capture-report.json` writes JSON. A
    file that parses as neither is an error rather than an empty list — an
    empty list stamps nothing and reports success, which is the silent failure
    this whole gate exists to avoid.
    """
    text = path.read_text(encoding="utf-8")
    stripped = text.lstrip()
    if stripped.startswith("[") or stripped.startswith("{"):
        parsed = json.loads(text)
        if isinstance(parsed, dict):
            parsed = parsed.get("produced", [])
        return {str(name) for name in parsed}
    names = {
        line.strip()
        for line in text.splitlines()
        if line.strip() and not line.lstrip().startswith("#")
    }
    # A path is accepted and reduced to its basename: the manifest keys on the
    # bare filename, and a caller echoing what it wrote has the full path.
    return {Path(name).name for name in names}


def resolve_produced(args: argparse.Namespace, manifest: dict,
                     images_dir: Path) -> set[str]:
    """What the run rendered, from whichever way the caller chose to say it.

    Raises `TypeError` when the caller said nothing. `--stamp` cannot default:
    guessing "everything" is the defect (tasks 642 and 816) and guessing
    "nothing" stamps no provenance while reporting success.
    """
    if args.produced:
        return {Path(name).name for name in args.produced}
    if args.produced_from is not None:
        return read_produced_list(args.produced_from)
    if args.produced_since is not None:
        return produced_since(images_dir, manifest, args.produced_since)
    raise TypeError(
        "--stamp needs to be told what this run rendered: pass --produced "
        "FILE (repeatable), --produced-from PATH, or --produced-since MARKER. "
        "It used to infer this from which digests had changed, which stamped "
        "this run's commit onto images it never rendered (tasks 642 and 816)")


def main(argv: list[str] | None = None) -> int:
    here = Path(__file__).resolve()
    default_manifest = here.parents[2] / "screenshots" / "image_manifest.json"

    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--stamp", action="store_true",
                      help="record provenance on the images this run rendered; "
                           "needs one of --produced/--produced-from/--produced-since")
    mode.add_argument("--report", action="store_true",
                      help="print generated images whose provenance is absent or stale")
    mode.add_argument("--check-pairs", action="store_true",
                      help="fail when a capture is behind its sibling in the "
                           "other appearance, or has no sibling (task 785)")
    parser.add_argument("--baseline", type=Path, default=None,
                        help="--check-pairs: JSON worklist of captures known to "
                             "be behind; the check fails on new ones and on "
                             "entries that no longer apply")
    parser.add_argument("--no-fail", action="store_true",
                        help="--check-pairs: report, but exit 0 -- for the "
                             "regeneration target, which renders one "
                             "appearance by design")
    # What the run rendered. Required by --stamp and mutually exclusive: the
    # tool must be told, and there is deliberately no blanket "all" -- a caller
    # that really did render everything can still say so by listing it, and
    # nothing in this pipeline is in a position to assert it otherwise (the
    # regeneration target renders one theme, so "every generated row" has never
    # been true of it). Tasks 642 and 816.
    produced = parser.add_mutually_exclusive_group()
    produced.add_argument("--produced", action="append", metavar="FILE",
                          help="a filename this run rendered; repeatable")
    produced.add_argument("--produced-from", type=Path, metavar="PATH",
                          help="file listing the rendered filenames, one per "
                               "line (# comments and blanks ignored), or a "
                               "JSON array of them")
    produced.add_argument("--produced-since", type=Path, metavar="MARKER",
                          help="treat generated images modified at or after "
                               "MARKER's mtime as this run's output")
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
            "dirty": render_paths_dirty(repo_root, images_dir),
        }
        try:
            rendered = resolve_produced(args, manifest, images_dir)
        except (TypeError, OSError) as error:
            print(f"capture_provenance: {error}", file=sys.stderr)
            return 2
        restamped = stamp(manifest, images_dir, provenance, rendered)
        # After stamp(), so a row it just gave a first record also gets this.
        seen = record_renders(manifest, images_dir, provenance["commit"], rendered)
        if restamped or seen:
            manifest_path.write_text(
                json.dumps(manifest, ensure_ascii=False, indent=2) + "\n",
                encoding="utf-8",
            )
        print(f"capture provenance: {len(restamped)} of {len(rendered)} "
              f"rendered image(s) restamped at {provenance['commit']} on "
              f"{provenance['platform']}"
              + (" (DIRTY TREE)" if provenance["dirty"] else ""))
        for name in restamped:
            print(f"  {name}")
        return 0

    if args.check_pairs:
        if repo_root is None:
            print("capture_provenance: not a git repository; pairs not checked",
                  file=sys.stderr)
            return 0 if args.no_fail else 1
        baseline = None
        if args.baseline is not None:
            baseline = json.loads(args.baseline.read_text(encoding="utf-8"))
        failures, notes = check_pairs(manifest, repo_root, images_dir, baseline)
        if notes:
            print(f"{len(notes)} capture(s) behind their sibling, carried by "
                  f"the baseline:")
            for line in notes:
                print(f"  {line}")
        if failures:
            print(f"{len(failures)} light/dark pair finding(s) - render the "
                  f"appearance that is behind (docs/ops/client-screenshots.md, "
                  f"\"Keeping the two appearances in step\"):")
            for line in failures:
                print(f"  {line}")
            return 0 if args.no_fail else 1
        print("no light/dark pair is behind its sibling"
              + (" beyond the baseline's worklist" if notes else ""))
        return 0

    lines = report(manifest, images_dir, repo_root)
    print("capture provenance by platform:")
    for line in platform_summary(manifest, images_dir):
        print(line)
    if lines:
        # Reported, never failed on: every image tracked before this existed is
        # legitimately unknown, so a non-zero count is the backlog rather than a
        # regression. Same rule as check_screenshots.py's owed set.
        images = len({line.split(":", 1)[0] for line in lines})
        print(f"{len(lines)} finding(s) across {images} image(s) with absent "
              f"or unverifiable provenance:")
        for line in lines:
            print(f"  {line}")
    else:
        print("every tracked generated image records a verifiable capture")
    return 0


if __name__ == "__main__":
    sys.exit(main())
