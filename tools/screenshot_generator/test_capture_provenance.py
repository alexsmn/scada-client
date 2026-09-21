#!/usr/bin/env python3
"""Tests for capture_provenance.py (task 434).

Runs in milliseconds and needs no build, no Qt and no generator binary: every
case builds its own manifest and image files in a temp dir. The git-dependent
paths are exercised against a real throwaway repository rather than a mock,
because the behaviour under test *is* what git reports — an ancestor check
against a rebased commit is the case the entry was filed about, and a fake
would only assert that the fake was called.
"""

import argparse
import hashlib
import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import capture_provenance as cp


def write_image(directory: Path, name: str, content: bytes) -> Path:
    path = directory / name
    path.write_bytes(content)
    return path


def sha(content: bytes) -> str:
    return hashlib.sha256(content).hexdigest()


def manifest_with(*rows: dict) -> dict:
    return {"images": list(rows)}


class GeneratorOwnership(unittest.TestCase):
    def test_auto_tags_are_owned(self):
        for tag in ("auto-view", "auto-dialog", "auto-menu"):
            self.assertTrue(cp.is_generator_owned(tag), tag)

    def test_hand_captured_tags_are_not(self):
        # A hand-captured image has no capture commit to record, so stamping
        # one would invent provenance rather than record it.
        for tag in ("manual-diagram", "manual-os", "manual-annotated"):
            self.assertFalse(cp.is_generator_owned(tag), tag)


class Stamping(unittest.TestCase):
    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        self.dir = Path(self._tmp.name)
        self.addCleanup(self._tmp.cleanup)
        self.provenance = {"commit": "abc1234", "platform": "windows", "dirty": False}

    def test_stamps_a_generated_image_that_has_no_provenance(self):
        write_image(self.dir, "a.png", b"one")
        m = manifest_with({"file": "a.png", "tag": "auto-view"})
        self.assertEqual(cp.stamp(m, self.dir, self.provenance, ["a.png"]), ["a.png"])
        self.assertEqual(
            m["images"][0]["captured"],
            {**self.provenance, "sha256": sha(b"one")},
        )

    def test_leaves_an_unchanged_image_alone(self):
        # The older commit is still the one that produced those bytes;
        # restamping would claim a freshness this render did not establish.
        write_image(self.dir, "a.png", b"one")
        old = {"commit": "old0000", "platform": "macos", "dirty": False,
               "sha256": sha(b"one")}
        m = manifest_with({"file": "a.png", "tag": "auto-view", "captured": dict(old)})
        self.assertEqual(cp.stamp(m, self.dir, self.provenance, ["a.png"]), [])
        self.assertEqual(m["images"][0]["captured"], old)

    def test_restamps_an_image_whose_bytes_changed(self):
        write_image(self.dir, "a.png", b"two")
        m = manifest_with({
            "file": "a.png", "tag": "auto-view",
            "captured": {"commit": "old0000", "platform": "macos",
                         "dirty": False, "sha256": sha(b"one")},
        })
        self.assertEqual(cp.stamp(m, self.dir, self.provenance, ["a.png"]), ["a.png"])
        self.assertEqual(m["images"][0]["captured"]["commit"], "abc1234")
        self.assertEqual(m["images"][0]["captured"]["sha256"], sha(b"two"))

    def test_never_stamps_a_hand_captured_image(self):
        write_image(self.dir, "hand.png", b"x")
        m = manifest_with({"file": "hand.png", "tag": "manual-diagram"})
        self.assertEqual(
            cp.stamp(m, self.dir, self.provenance, ["hand.png"]), [])
        self.assertNotIn("captured", m["images"][0])

    def test_skips_a_row_whose_file_is_not_on_disk(self):
        # Published-elsewhere and not-yet-rendered rows are normal; they are
        # not a reason to fail or to invent a digest.
        m = manifest_with({"file": "absent.png", "tag": "auto-view"})
        self.assertEqual(
            cp.stamp(m, self.dir, self.provenance, ["absent.png"]), [])
        self.assertNotIn("captured", m["images"][0])


class StampsOnlyWhatTheRunRendered(unittest.TestCase):
    """The tasks 642 and 816 gate.

    Every case in `Stamping` above hands `stamp()` a one-row manifest, so
    "restamp what moved" and "restamp what I rendered" agree on all of them --
    which is exactly why the suite was green while the defect was live. The
    reproduction needs two rows where the run produced one.
    """

    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        self.dir = Path(self._tmp.name)
        self.addCleanup(self._tmp.cleanup)
        self.provenance = {"commit": "abc1234", "platform": "macos", "dirty": False}

    def test_a_row_this_run_did_not_render_keeps_its_own_provenance(self):
        # The reported failure: a partial render in a checkout where some other
        # capture is modified -- a peer's uncommitted re-render, or leftovers
        # from an earlier pass. Its digest differs from its record, so the old
        # code stamped this run's commit onto an image this run never touched.
        write_image(self.dir, "mine.png", b"fresh")
        write_image(self.dir, "peers.png", b"somebody elses render")
        old = {"commit": "old0000", "platform": "macos", "dirty": False,
               "sha256": sha(b"the bytes that were stamped")}
        m = manifest_with(
            {"file": "mine.png", "tag": "auto-view"},
            {"file": "peers.png", "tag": "auto-view", "captured": dict(old)},
        )
        self.assertEqual(
            cp.stamp(m, self.dir, self.provenance, ["mine.png"]), ["mine.png"])
        self.assertEqual(m["images"][1]["captured"], old)

    def test_an_unrendered_row_with_no_record_is_not_backfilled(self):
        # The other half, dormant today only because every generated row on
        # disk happens to be stamped: `None != digest` is true for every
        # unstamped image, so a row added to the manifest before its first
        # render was one partial `--stamp` away from a manufactured baseline.
        write_image(self.dir, "mine.png", b"fresh")
        write_image(self.dir, "never-rendered.png", b"placed by hand")
        m = manifest_with(
            {"file": "mine.png", "tag": "auto-view"},
            {"file": "never-rendered.png", "tag": "auto-view"},
        )
        cp.stamp(m, self.dir, self.provenance, ["mine.png"])
        self.assertNotIn("captured", m["images"][1])

    def test_a_rendered_row_with_no_record_does_get_its_first_one(self):
        # Not a backfill, and the distinction is the whole correctness of the
        # function: this run is what made those bytes.
        write_image(self.dir, "a.png", b"one")
        m = manifest_with({"file": "a.png", "tag": "auto-view"})
        self.assertEqual(
            cp.stamp(m, self.dir, self.provenance, ["a.png"]), ["a.png"])
        self.assertEqual(m["images"][0]["captured"]["commit"], "abc1234")

    def test_a_rendered_row_whose_bytes_are_identical_keeps_the_older_commit(self):
        # That commit did produce those bytes; restamping would claim a
        # freshness this render did not establish.
        write_image(self.dir, "a.png", b"one")
        old = {"commit": "old0000", "platform": "macos", "dirty": False,
               "sha256": sha(b"one")}
        m = manifest_with({"file": "a.png", "tag": "auto-view",
                           "captured": dict(old)})
        self.assertEqual(cp.stamp(m, self.dir, self.provenance, ["a.png"]), [])
        self.assertEqual(m["images"][0]["captured"], old)

    def test_stamping_nothing_is_a_no_op_not_a_whole_gallery(self):
        write_image(self.dir, "a.png", b"changed")
        write_image(self.dir, "b.png", b"changed too")
        m = manifest_with(
            {"file": "a.png", "tag": "auto-view"},
            {"file": "b.png", "tag": "auto-view"},
        )
        self.assertEqual(cp.stamp(m, self.dir, self.provenance, []), [])
        self.assertNotIn("captured", m["images"][0])
        self.assertNotIn("captured", m["images"][1])

    def test_being_told_nothing_at_all_raises_rather_than_guessing(self):
        # Loud in both directions: defaulting to "everything" restores the
        # defect, defaulting to "nothing" silently stops recording provenance.
        write_image(self.dir, "a.png", b"one")
        m = manifest_with({"file": "a.png", "tag": "auto-view"})
        with self.assertRaises(TypeError):
            cp.stamp(m, self.dir, self.provenance, None)
        self.assertNotIn("captured", m["images"][0])


class Reporting(unittest.TestCase):
    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        self.dir = Path(self._tmp.name)
        self.addCleanup(self._tmp.cleanup)

    def test_unstamped_image_reads_as_unknown_not_as_fresh(self):
        write_image(self.dir, "a.png", b"one")
        m = manifest_with({"file": "a.png", "tag": "auto-view"})
        lines = cp.report(m, self.dir, None)
        self.assertEqual(len(lines), 1)
        self.assertIn("provenance unknown", lines[0])

    def test_changed_bytes_make_a_recorded_commit_stale(self):
        # The defect the entry is about: the image moved and nothing said so.
        write_image(self.dir, "a.png", b"changed")
        m = manifest_with({
            "file": "a.png", "tag": "auto-view",
            "captured": {"commit": "abc1234", "platform": "macos",
                         "dirty": False, "sha256": sha(b"one")},
        })
        lines = cp.report(m, self.dir, None)
        self.assertEqual(len(lines), 1)
        self.assertIn("STALE", lines[0])
        self.assertIn("abc1234", lines[0])

    def test_a_dirty_capture_is_reported_even_when_its_digest_matches(self):
        write_image(self.dir, "a.png", b"one")
        m = manifest_with({
            "file": "a.png", "tag": "auto-view",
            "captured": {"commit": "abc1234", "platform": "macos",
                         "dirty": True, "sha256": sha(b"one")},
        })
        lines = cp.report(m, self.dir, None)
        self.assertEqual(len(lines), 1)
        self.assertIn("dirty tree", lines[0])

    def test_a_dirty_capture_is_still_checked_against_history(self):
        # Task 815. `dirty` and "not in this history" are independent facts --
        # one about the worktree a capture came from, the other about whether
        # the commit it names still exists -- and report() used to return on
        # the first. Because render_paths_dirty() examined `client` while the
        # gallery lives under `client/`, 144 of the 150 stamped rows carried a
        # dirty flag that was never true, so the history check below had never
        # run on a row that reached it and the report read as passing.
        #
        # The git half is real rather than faked: an abandoned commit is what
        # a rebase-heavy shared checkout actually produces.
        with tempfile.TemporaryDirectory() as tmp:
            repo = Path(tmp)
            run = lambda *a: subprocess.run(("git", *a), cwd=repo, check=True,
                                            capture_output=True)
            run("init", "-q", "-b", "main")
            run("config", "user.email", "t@example.com")
            run("config", "user.name", "t")
            (repo / "f.txt").write_text("one")
            run("add", "-A")
            run("commit", "-qm", "one")
            run("checkout", "-q", "-b", "side")
            (repo / "f.txt").write_text("side")
            run("commit", "-qam", "side")
            abandoned = cp.head_commit(repo)
            run("checkout", "-q", "main")

            write_image(self.dir, "a.png", b"one")
            m = manifest_with({
                "file": "a.png", "tag": "auto-view",
                "captured": {"commit": abandoned, "platform": "macos",
                             "dirty": True, "sha256": sha(b"one")},
            })
            lines = cp.report(m, self.dir, repo)

        self.assertEqual(len(lines), 2, lines)
        self.assertIn("dirty tree", lines[0])
        self.assertIn("not in this history", lines[1])

    def test_an_unstamped_row_stops_before_the_history_check(self):
        # The two checks above `dirty` DO still stop, and for a reason it does
        # not share: an unstamped row has no commit to ask about at all.
        write_image(self.dir, "a.png", b"one")
        m = manifest_with({"file": "a.png", "tag": "auto-view"})
        lines = cp.report(m, self.dir, Path("/nonexistent"))
        self.assertEqual(len(lines), 1)
        self.assertIn("provenance unknown", lines[0])

    def test_a_clean_verifiable_capture_reports_nothing(self):
        write_image(self.dir, "a.png", b"one")
        m = manifest_with({
            "file": "a.png", "tag": "auto-view",
            "captured": {"commit": "abc1234", "platform": "macos",
                         "dirty": False, "sha256": sha(b"one")},
        })
        self.assertEqual(cp.report(m, self.dir, None), [])

    def test_hand_captured_images_are_not_reported(self):
        write_image(self.dir, "hand.png", b"x")
        m = manifest_with({"file": "hand.png", "tag": "manual-os"})
        self.assertEqual(cp.report(m, self.dir, None), [])

    def test_platform_summary_counts_unknown_separately(self):
        write_image(self.dir, "a.png", b"one")
        write_image(self.dir, "b.png", b"two")
        write_image(self.dir, "c.png", b"three")
        m = manifest_with(
            {"file": "a.png", "tag": "auto-view",
             "captured": {"commit": "c", "platform": "macos", "dirty": False,
                          "sha256": sha(b"one")}},
            {"file": "b.png", "tag": "auto-view",
             "captured": {"commit": "c", "platform": "windows", "dirty": False,
                          "sha256": sha(b"two")}},
            {"file": "c.png", "tag": "auto-view"},
        )
        self.assertEqual(
            cp.platform_summary(m, self.dir),
            ["  macos: 1", "  unknown: 1", "  windows: 1"],
        )


class SayingWhatTheRunRendered(unittest.TestCase):
    """The three ways a caller answers, and the refusal when it does not."""

    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        self.dir = Path(self._tmp.name)
        self.addCleanup(self._tmp.cleanup)

    def _args(self, **kw):
        defaults = {"produced": None, "produced_from": None, "produced_since": None}
        return argparse.Namespace(**{**defaults, **kw})

    def test_explicit_names_are_taken_as_given(self):
        m = manifest_with({"file": "a.png", "tag": "auto-view"})
        self.assertEqual(
            cp.resolve_produced(self._args(produced=["a.png"]), m, self.dir),
            {"a.png"})

    def test_a_path_is_reduced_to_the_basename_the_manifest_keys_on(self):
        # A caller echoing what it wrote has the full path, and the manifest
        # has the bare filename.
        m = manifest_with({"file": "a.png", "tag": "auto-view"})
        self.assertEqual(
            cp.resolve_produced(
                self._args(produced=["client/screenshots/a.png"]), m, self.dir),
            {"a.png"})

    def test_a_list_file_ignores_blanks_and_comments(self):
        listing = self.dir / "produced.txt"
        listing.write_text("# rendered by this pass\na.png\n\nb.png\n")
        self.assertEqual(cp.read_produced_list(listing), {"a.png", "b.png"})

    def test_a_json_array_is_accepted_too(self):
        # The shape web's capture-report.json already writes, so a port of
        # that record needs no second reader.
        listing = self.dir / "produced.json"
        listing.write_text('["a.png", "b.png"]')
        self.assertEqual(cp.read_produced_list(listing), {"a.png", "b.png"})

    def test_a_json_object_is_read_from_its_produced_key(self):
        listing = self.dir / "capture-report.json"
        listing.write_text('{"produced": ["a.png"], "skipped": ["b.png"]}')
        self.assertEqual(cp.read_produced_list(listing), {"a.png"})

    def test_saying_nothing_raises_rather_than_defaulting(self):
        m = manifest_with({"file": "a.png", "tag": "auto-view"})
        with self.assertRaises(TypeError):
            cp.resolve_produced(self._args(), m, self.dir)

    def test_produced_since_takes_what_the_pass_rewrote_and_leaves_the_rest(self):
        write_image(self.dir, "before.png", b"older")
        marker = self.dir / "marker"
        marker.write_bytes(b"")
        # Push the untouched image firmly behind the marker rather than
        # relying on the filesystem's timestamp granularity to separate them.
        import os
        old = marker.stat().st_mtime - 60
        os.utime(self.dir / "before.png", (old, old))
        write_image(self.dir, "after.png", b"rendered by this pass")
        m = manifest_with(
            {"file": "before.png", "tag": "auto-view"},
            {"file": "after.png", "tag": "auto-view"},
        )
        self.assertEqual(
            cp.produced_since(self.dir, m, marker), {"after.png"})

    def test_produced_since_never_reaches_a_hand_captured_row(self):
        write_image(self.dir, "hand.png", b"x")
        marker = self.dir / "marker"
        marker.write_bytes(b"")
        import os
        old = marker.stat().st_mtime - 60
        os.utime(marker, (old, old))
        m = manifest_with({"file": "hand.png", "tag": "manual-diagram"})
        self.assertEqual(cp.produced_since(self.dir, m, marker), set())


class GitInteraction(unittest.TestCase):
    """The ancestor and dirty checks, against a real throwaway repository."""

    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        self.repo = Path(self._tmp.name)
        self.addCleanup(self._tmp.cleanup)
        run = lambda *a: subprocess.run(("git", *a), cwd=self.repo, check=True,
                                        capture_output=True)
        run("init", "-q", "-b", "main")
        run("config", "user.email", "t@example.com")
        run("config", "user.name", "t")
        (self.repo / "client").mkdir()
        (self.repo / "client" / "f.txt").write_text("one")
        run("add", "-A")
        run("commit", "-qm", "one")
        self.first = cp.head_commit(self.repo)

    def _commit(self, text: str) -> str:
        (self.repo / "client" / "f.txt").write_text(text)
        subprocess.run(("git", "commit", "-qam", text), cwd=self.repo, check=True,
                       capture_output=True)
        return cp.head_commit(self.repo)

    def test_an_ancestor_commit_is_in_history(self):
        self._commit("two")
        self.assertTrue(cp.commit_is_in_history(self.repo, self.first))

    def test_a_commit_from_an_abandoned_branch_is_not(self):
        # The neither-side-correct case: a capture stamped on work that was
        # rebased away names a tree this history does not contain.
        subprocess.run(("git", "checkout", "-q", "-b", "side"), cwd=self.repo,
                       check=True, capture_output=True)
        abandoned = self._commit("side")
        subprocess.run(("git", "checkout", "-q", "main"), cwd=self.repo,
                       check=True, capture_output=True)
        self.assertFalse(cp.commit_is_in_history(self.repo, abandoned))

    def test_an_unknown_commit_is_not_in_history(self):
        self.assertFalse(cp.commit_is_in_history(self.repo, "0" * 40))

    def test_dirt_under_a_render_path_is_dirty(self):
        (self.repo / "client" / "f.txt").write_text("modified")
        self.assertTrue(cp.render_paths_dirty(self.repo))

    def test_dirt_outside_the_render_paths_is_not(self):
        # The shared checkout always has unrelated edits in flight; treating
        # those as dirt would mark every capture unreproducible and the flag
        # would carry no information.
        (self.repo / "unrelated.md").write_text("x")
        subprocess.run(("git", "add", "-A"), cwd=self.repo, check=True,
                       capture_output=True)
        self.assertFalse(cp.render_paths_dirty(self.repo))

    def test_a_clean_tree_is_not_dirty(self):
        self.assertFalse(cp.render_paths_dirty(self.repo))

    def test_the_gallery_it_writes_does_not_count_as_dirt(self):
        # The regression this pins: the gallery lives under `client/`, which is
        # a render path, so the act of regenerating dirtied the tree the flag
        # was asking about. render_paths_dirty() could not return False during
        # a stamp, and all 150 stamped rows read "dirty": true as a result --
        # which also meant report() returned on the dirty branch and never ran
        # its "not in this history" check for any image.
        gallery = self.repo / "client" / "screenshots"
        gallery.mkdir(parents=True, exist_ok=True)
        (gallery / "devices.png").write_bytes(b"rendered")
        self.assertFalse(cp.render_paths_dirty(self.repo, gallery))
        # Still dirty when the SOURCE moved, which is the case worth reporting.
        (self.repo / "client" / "f.txt").write_text("modified")
        self.assertTrue(cp.render_paths_dirty(self.repo, gallery))


if __name__ == "__main__":
    unittest.main()
