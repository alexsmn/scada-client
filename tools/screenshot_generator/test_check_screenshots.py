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



class UncheckedCapturesTest(unittest.TestCase):
    """What a clean run is *not* evidence about.

    The check verifies the `auto-*` rows the fixture names, prints "N captures
    checked, 0 error(s)", and used to stop there -- so a pass over well under
    half the gallery read as a verdict on all of it. These pin the three sets
    that sit outside the checked one, because each is invisible in a different
    way and only one of them was ever reported.
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

    # reshell-theme rows are outside the auto-* set entirely, so neither the
    # checked count nor the owed count ever mentioned them -- 28 captures with
    # no structural coverage at all, invisible in every line the check printed.
    def test_a_reshell_theme_row_is_reported_as_themed_only(self) -> None:
        self.write_source("")
        self.assertEqual(
            self.unchecked(manifest(("object-tree.png", "reshell-theme")), {}, set()),
            {"themed-only": ["object-tree.png"]},
        )

    # A hand-maintained image is not the generator's to check or to owe.
    def test_an_unmanaged_row_is_not_reported(self) -> None:
        self.write_source("")
        self.assertEqual(
            self.unchecked(manifest(("architecture.png", "manual")), {}, set()), {}
        )


class ThemedCaptureSelectionTest(unittest.TestCase):
    """The themed pass's row list, which used to be written out by hand.

    `client_screenshot_check_themed` carried a 25-name `--only` list in
    CMakeLists.txt beside a comment asking for it to be kept in sync with the
    manifest. It was three names behind, so debugger.png, frame-decode-pane.png
    and watch-filter-bar.png were rendered by nothing at all -- and because that
    ctest asserted nothing structural, nothing could say so (backlog 630).
    Deriving the list is the fix; these cases are what stop it being unpicked.
    """

    def test_selects_every_reshell_theme_row(self) -> None:
        self.assertEqual(
            check.themed_captures(
                manifest(
                    ("object-tree.png", "reshell-theme"),
                    ("debugger.png", "reshell-theme"),
                )
            ),
            ["debugger.png", "object-tree.png"],
        )

    def test_excludes_every_other_tag(self) -> None:
        # auto-* belongs to the default pass and `manual` to nobody here.
        self.assertEqual(
            check.themed_captures(
                manifest(
                    ("table.png", "auto-view"),
                    ("client-login.png", "auto-dialog"),
                    ("architecture.png", "manual"),
                )
            ),
            [],
        )

    def test_is_sorted_so_the_only_list_is_stable(self) -> None:
        # The list becomes a --only argument; an unstable order would churn.
        self.assertEqual(
            check.themed_captures(
                manifest(
                    ("z.png", "reshell-theme"),
                    ("a.png", "reshell-theme"),
                    ("m.png", "reshell-theme"),
                )
            ),
            ["a.png", "m.png", "z.png"],
        )

    def test_tracks_the_real_manifest_rather_than_a_copied_list(self) -> None:
        # The regression itself: whatever the tree's manifest tags today is
        # what the themed pass renders, with nothing to keep in sync by hand.
        import json

        here = Path(__file__).resolve().parent
        real = json.loads(
            (here / ".." / ".." / "screenshots" / "image_manifest.json").read_text(
                encoding="utf-8"
            )
        )
        expected = sorted(
            e["file"] for e in real["images"] if e["tag"] == "reshell-theme"
        )
        self.assertEqual(check.themed_captures(real), expected)
        self.assertTrue(expected, "the manifest should carry reshell-theme rows")


if __name__ == "__main__":
    unittest.main()
