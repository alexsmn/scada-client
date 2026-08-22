#!/usr/bin/env python3
"""Tests for the screenshot check's owed-set query.

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
        # renders from CaptureSettingsDialog, which names its file in C++ and
        # not in the fixture, and was reported owed for months.
        self.write_source(
            '  constexpr const char* kFilename = "settings-dialog.png";\n'
        )
        self.assertEqual(
            self.owed(manifest(("settings-dialog.png", "auto-dialog")), {}), {}
        )

    def test_unmanaged_tags_are_never_owed(self) -> None:
        # Hand-captured and themed rows are not the generator's debt:
        # `manual-*` is captured by a person, and `reshell-theme` is rendered
        # by the themed pass rather than by the run this script checks.
        self.write_source("")
        self.assertEqual(
            self.owed(
                manifest(
                    ("client-window.png", "manual-modus"),
                    ("hardware-tree.png", "reshell-theme"),
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


if __name__ == "__main__":
    unittest.main()
