#!/usr/bin/env python3
"""Tests for the screenshot check's coverage reporting.

The owed set is "manifest rows the docs pipeline manages that this generator
does not produce" — the remaining work of task 39. It was derived by hand for
months and was wrong twice, both times in the same direction: a capture that
had started rendering from a filename hardcoded in the generator's C++ went on
being counted as owed, because the derivation only ever looked at
screenshot_data.json. settings-dialog.png sat in the count that way for
months. The cases below pin both source of truth and both exclusions.

Run directly, or via ctest as `client_screenshot_owed_tests`.
"""

from __future__ import annotations

import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import check_screenshots as check  # noqa: E402


def manifest(*rows: tuple[str, str]) -> dict:
    return {"images": [{"file": f, "tag": t} for f, t in rows]}


class OwedCapturesTest(unittest.TestCase):
    def setUp(self) -> None:
        self._temp = tempfile.TemporaryDirectory(prefix="scada_owed_test_")
        self.sources = Path(self._temp.name)
        self.addCleanup(self._temp.cleanup)

    def write_source(self, text: str) -> None:
        (self.sources / "some_capture.cpp").write_text(text, encoding="utf-8")

    def owed(self, images: dict, data: dict) -> dict[str, list[str]]:
        return check.owed_captures(images, data, self.sources)

    def test_a_managed_row_no_capture_produces_is_owed(self) -> None:
        self.write_source("// nothing renders anything here\n")
        self.assertEqual(
            self.owed(manifest(("menu-parameters.png", "auto-menu")), {}),
            {"auto-menu": ["menu-parameters.png"]},
        )

    def test_a_row_the_fixture_renders_is_not_owed(self) -> None:
        self.write_source("")
        data = {"screenshots": [{"filename": "table.png"}]}
        self.assertEqual(self.owed(manifest(("table.png", "auto-view")), data), {})

    def test_a_dialog_row_the_fixture_renders_is_not_owed(self) -> None:
        self.write_source("")
        data = {"dialogs": [{"filename": "limits.png"}]}
        self.assertEqual(self.owed(manifest(("limits.png", "auto-dialog")), data), {})

    def test_a_row_hardcoded_in_the_generator_is_not_owed(self) -> None:
        # The regression the count kept getting wrong: settings-dialog.png
        # renders from CaptureSettingsPanel, which names its file in C++ and
        # not in the fixture, and was reported owed for months.
        self.write_source(
            '  constexpr const char* kFilename = "settings-dialog.png";\n'
        )
        self.assertEqual(
            self.owed(manifest(("settings-dialog.png", "auto-dialog")), {}), {}
        )

    def test_unmanaged_tags_are_never_owed(self) -> None:
        # A hand-captured image is not the generator's debt: a person takes it.
        self.write_source("")
        self.assertEqual(
            self.owed(
                manifest(
                    ("client-window.png", "manual-modus"),
                    ("menu-notepad.png", "manual-os"),
                ),
                {},
            ),
            {},
        )

    def test_owed_rows_group_by_tag_and_sort(self) -> None:
        self.write_source("")
        self.assertEqual(
            self.owed(
                manifest(
                    ("ti-formula.png", "auto-dialog"),
                    ("menu-scheme.png", "auto-menu"),
                    ("ti-channel.png", "auto-dialog"),
                ),
                {},
            ),
            {
                "auto-dialog": ["ti-channel.png", "ti-formula.png"],
                "auto-menu": ["menu-scheme.png"],
            },
        )


class HardcodedFilenamesTest(unittest.TestCase):
    def setUp(self) -> None:
        self._temp = tempfile.TemporaryDirectory(prefix="scada_owed_test_")
        self.sources = Path(self._temp.name)
        self.addCleanup(self._temp.cleanup)

    def test_reads_both_headers_and_sources(self) -> None:
        (self.sources / "a.cpp").write_text('"one.png"', encoding="utf-8")
        (self.sources / "b.h").write_text('"two.png"', encoding="utf-8")
        self.assertEqual(
            check.hardcoded_capture_filenames(self.sources),
            {"one.png", "two.png"},
        )

    def test_ignores_non_png_literals(self) -> None:
        (self.sources / "a.cpp").write_text(
            '"screenshot_data.json" "res/icon.svg" "real.png"', encoding="utf-8"
        )
        self.assertEqual(
            check.hardcoded_capture_filenames(self.sources), {"real.png"}
        )



class UncheckedCapturesTest(unittest.TestCase):
    """What a clean run is *not* evidence about.

    The check compares the `auto-*` rows the fixture names against their specs,
    prints "N captures checked, 0 error(s)", and used to stop there -- so a pass
    over well under half the gallery read as a verdict on all of it. These pin
    the sets that sit outside the dimension-checked one, because each is
    invisible in a different way and only one of them was ever reported.
    """

    def setUp(self) -> None:
        self._temp = tempfile.TemporaryDirectory(prefix="scada_unchecked_test_")
        self.sources = Path(self._temp.name)
        self.addCleanup(self._temp.cleanup)

    def write_source(self, text: str) -> None:
        (self.sources / "some_capture.cpp").write_text(text, encoding="utf-8")

    def unchecked(self, images, data, checked) -> dict[str, list[str]]:
        return check.unchecked_captures(images, data, self.sources, set(checked))

    def test_a_checked_row_is_not_reported(self) -> None:
        self.write_source("")
        data = {"screenshots": [{"filename": "table.png"}]}
        self.assertEqual(
            self.unchecked(manifest(("table.png", "auto-view")), data, {"table.png"}),
            {},
        )

    # The set that fell through both reports: the generator writes it, so the
    # owed query correctly excludes it, and no fixture spec names it, so the
    # dimension pass never looks at it either.
    def test_a_hardcoded_row_the_fixture_does_not_name_is_produced_unchecked(
        self,
    ) -> None:
        self.write_source('constexpr const char* kFilename = "settings-dialog.png";')
        self.assertEqual(
            self.unchecked(manifest(("settings-dialog.png", "auto-dialog")), {}, set()),
            {"produced-unchecked": ["settings-dialog.png"]},
        )

    # An owed row is already reported as owed; repeating it here would say the
    # backlog twice and bury the set that nothing else mentions.
    def test_an_owed_row_is_left_to_the_owed_report(self) -> None:
        self.write_source("// nothing renders anything here\n")
        self.assertEqual(
            self.unchecked(manifest(("menu-parameters.png", "auto-menu")), {}, set()),
            {},
        )

    # A fixture spec that did not appear is an error, not a silent gap, so it
    # must not be quietly re-filed as something this pass merely skipped.
    def test_a_fixture_row_that_failed_to_appear_is_left_to_the_error(self) -> None:
        self.write_source("")
        data = {"screenshots": [{"filename": "table.png"}]}
        self.assertEqual(
            self.unchecked(manifest(("table.png", "auto-view")), data, set()), {}
        )

    # A hand-maintained image is not the generator's to check or to owe.
    def test_an_unmanaged_row_is_not_reported(self) -> None:
        self.write_source("")
        self.assertEqual(
            self.unchecked(manifest(("architecture.png", "manual")), {}, set()), {}
        )


class HardcodedCaptureCoverageTest(unittest.TestCase):
    """Existence coverage for managed rows the fixture does not name.

    These were the `reshell-theme` rows, rendered by a second ctest off a
    25-name `--only` list hand-written in CMakeLists.txt beside a comment asking
    for it to be kept in sync. It was three names behind, so debugger.png,
    frame-decode-pane.png and watch-filter-bar.png were rendered by nothing at
    all, and that ctest asserted nothing structural, so nothing could say so
    (backlog 630). With the opt-in theme gone the rows are ordinary `auto-*`
    ones and the single pass covers them -- by scanning the generator's own
    sources for the filenames, so there is still no list to keep in sync.
    """

    def setUp(self) -> None:
        self._temp = tempfile.TemporaryDirectory(prefix="scada_hardcoded_test_")
        self.sources = Path(self._temp.name)
        self.addCleanup(self._temp.cleanup)

    def write_source(self, text: str) -> None:
        (self.sources / "some_capture.cpp").write_text(text, encoding="utf-8")

    def test_finds_a_filename_written_in_the_c_plus_plus(self) -> None:
        self.write_source(
            'constexpr const char* kFilename = "workbench-activity-rail.png";'
        )
        self.assertEqual(
            check.hardcoded_capture_filenames(self.sources),
            {"workbench-activity-rail.png"},
        )

    def test_finds_every_such_filename_in_one_file(self) -> None:
        self.write_source(
            'auto a = "debugger.png";\nauto b = "frame-decode-pane.png";\n'
        )
        self.assertEqual(
            check.hardcoded_capture_filenames(self.sources),
            {"debugger.png", "frame-decode-pane.png"},
        )

    def test_the_three_rows_that_were_rendered_by_nothing_are_now_covered(
        self,
    ) -> None:
        # The regression itself: each is a manifest-managed row the fixture may
        # or may not name, and every one of them must be reachable from the
        # generator's own sources or the single pass would go back to missing
        # them.
        import json

        here = Path(__file__).resolve().parent
        manifest_path = here / ".." / ".." / "screenshots" / "image_manifest.json"
        real = json.loads(manifest_path.read_text(encoding="utf-8"))
        managed = {
            e["file"] for e in real["images"] if e["tag"].startswith("auto-")
        }
        data = json.loads(
            (here / "screenshot_data.json").read_text(encoding="utf-8")
        )
        named = {
            spec["filename"]
            for key in ("screenshots", "dialogs")
            for spec in data.get(key, [])
        }
        reachable = named | check.hardcoded_capture_filenames(here)
        for filename in (
            "debugger.png",
            "frame-decode-pane.png",
            "watch-filter-bar.png",
        ):
            self.assertIn(filename, managed)
            self.assertIn(filename, reachable)


if __name__ == "__main__":
    unittest.main()
