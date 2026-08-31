#!/usr/bin/env python3
"""Tests for capture_provenance.py (task 434).

Runs in milliseconds and needs no build, no Qt and no generator binary: every
case builds its own manifest and image files in a temp dir. The git-dependent
paths are exercised against a real throwaway repository rather than a mock,
because the behaviour under test *is* what git reports — an ancestor check
against a rebased commit is the case the entry was filed about, and a fake
would only assert that the fake was called.
"""

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
        self.assertEqual(cp.stamp(m, self.dir, self.provenance), ["a.png"])
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
        self.assertEqual(cp.stamp(m, self.dir, self.provenance), [])
        self.assertEqual(m["images"][0]["captured"], old)

    def test_restamps_an_image_whose_bytes_changed(self):
        write_image(self.dir, "a.png", b"two")
        m = manifest_with({
            "file": "a.png", "tag": "auto-view",
            "captured": {"commit": "old0000", "platform": "macos",
                         "dirty": False, "sha256": sha(b"one")},
        })
        self.assertEqual(cp.stamp(m, self.dir, self.provenance), ["a.png"])
        self.assertEqual(m["images"][0]["captured"]["commit"], "abc1234")
        self.assertEqual(m["images"][0]["captured"]["sha256"], sha(b"two"))

    def test_never_stamps_a_hand_captured_image(self):
        write_image(self.dir, "hand.png", b"x")
        m = manifest_with({"file": "hand.png", "tag": "manual-diagram"})
        self.assertEqual(cp.stamp(m, self.dir, self.provenance), [])
        self.assertNotIn("captured", m["images"][0])

    def test_skips_a_row_whose_file_is_not_on_disk(self):
        # Published-elsewhere and not-yet-rendered rows are normal; they are
        # not a reason to fail or to invent a digest.
        m = manifest_with({"file": "absent.png", "tag": "auto-view"})
        self.assertEqual(cp.stamp(m, self.dir, self.provenance), [])
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


if __name__ == "__main__":
    unittest.main()
