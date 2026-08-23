#!/usr/bin/env python3
"""Behaviour tests for `check_untranslated_ui_strings.py`.

A checker whose regexes silently stop matching does not fail — it *passes*,
reporting OK over a tree full of the defect it exists to catch. That is not a
hypothetical failure mode for this one: for months its literal pattern was
`u"..."` alone, so every `L"..."`, every plain `"..."` at a Qt widget and every
raw `R"(...)"` block went unreported, and the check said OK the whole time.

So each case below pins one form the checker must see, and each negative pins
one shape of correct code it must stay quiet about. The negatives matter as
much as the positives: the widening that made the positives work also made
`Translate("...")` itself look like a finding, and a checker that reports 40
correct call sites gets switched off.

Source-only and dependency-free, like the checker it covers. Run it directly or
through ctest as `client_untranslated_string_check_test`.
"""

import contextlib
import io
import pathlib
import sys
import tempfile

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

import check_untranslated_ui_strings as checker  # noqa: E402

# (name, source) — each must produce at least one finding from the named rule.
DIALOG_SINK_CASES = (
    (
        "wide literal formatted at the throw site (task 351's form)",
        'void f(){ ShowMessageBox(e, s, u16format(L"Widget failed to load.")); }',
    ),
    (
        "plain literal at a window title",
        'void f(){ setWindowTitle("Device Setup"); }',
    ),
    (
        "u8 literal at a resource error",
        'void f(){ throw ResourceError{u8"Cannot open the report."}; }',
    ),
    (
        "raw literal at a message box",
        'void f(){ ShowMessageBox(e, s, R"(Import failed.)"); }',
    ),
    (
        "adjacent wide literals are judged as one string",
        'void f(){ ShowMessageBox(e, s, L"Invalid address: " L"%1."); }',
    ),
    (
        "narrow character-array constant reaching a sink",
        'const char kTitle[] = "Export report";\nvoid f(){ setWindowTitle(kTitle); }',
    ),
)

DISPLAY_CASES = (
    (
        "plain literal painted with drawText",
        'void f(QPainter& p){ p.drawText(r, Qt::AlignLeft, "No data available."); }',
    ),
    (
        "tooltip literal",
        'void f(){ b->setToolTip("Zoom out"); }',
    ),
    (
        "file-scope literal table indexed by a paint loop (task 418's form)",
        'const Col kCols[] = {{"Current", 1}, {"Average", 2}};\n'
        "void f(QPainter& p){ p.drawText(r, 0, QString::fromUtf8(kCols[0].name)); }",
    ),
)

# Correct code, or text no operator reads. None may produce a finding.
QUIET_CASES = (
    ("Translate() call", 'void f(){ setWindowTitle(Translate("Device Setup")); }'),
    ("Tr() per-file wrapper", 'void f(){ b->setToolTip(Tr("Zoom out")); }'),
    ("Qt tr() call", 'void f(){ setWindowTitle(tr("Device Setup")); }'),
    # The whole mark-for-translation macro family, not just the one spelling
    # this tree uses today. A checker that knows QT_TRANSLATE_NOOP and reports
    # QT_TR_NOOP as a defect would repeat, in miniature, the bug it was fixed
    # for.
    ("QT_TR_NOOP macro", 'void f(){ setWindowTitle(QT_TR_NOOP("Device Setup")); }'),
    (
        "QT_TRANSLATE_NOOP macro",
        'void f(){ setWindowTitle(QT_TRANSLATE_NOOP("Ctx", "Device Setup")); }',
    ),
    (
        "QT_TRANSLATE_NOOP3 macro",
        'void f(){ setWindowTitle(QT_TRANSLATE_NOOP3("Ctx", "Device Setup", "c")); }',
    ),
    ("markup and placeholders only", 'void f(){ l->setText("<p>%1</p>"); }'),
    (
        "literal table in a file that paints nothing",
        'const char* kIds[] = {"ns=2;s=Tag.A", "Alpha"};',
    ),
    ("literal that reaches no sink at all", 'void f(){ send("HELLO SERVER"); }'),
    ("literal with no letters", 'void f(){ setWindowTitle(" - "); }'),
)


def dialog_findings(source: str, directory: pathlib.Path):
    path = directory / "case.cpp"
    path.write_text(source, encoding="utf-8")
    return [text for _, _, text, _ in checker.scan_file(path, directory)]


def display_findings(source: str):
    return [text for _, _, text in checker.scan_file_for_display_literals(source)]


def main() -> int:
    failures = []
    with tempfile.TemporaryDirectory() as tmp:
        directory = pathlib.Path(tmp)

        for name, source in DIALOG_SINK_CASES:
            if not dialog_findings(source, directory):
                failures.append(f"rule 1 missed: {name}")

        for name, source in DISPLAY_CASES:
            if not display_findings(source):
                failures.append(f"rule 3 missed: {name}")

        # The joined form must come back whole, not as its two pieces.
        joined = dialog_findings(
            'void f(){ ShowMessageBox(e, s, L"Invalid address: " L"%1."); }', directory
        )
        if joined != ["Invalid address: %1."]:
            failures.append(f"adjacent literals not joined: {joined}")

        # Literals in *different* arguments must stay separate; welding them
        # produced findings quoting text no operator ever sees.
        separate = dialog_findings(
            'void f(){ ShowMessageBox(e, s, L"No data to export.", L"Export"); }',
            directory,
        )
        if separate != ["No data to export.", "Export"]:
            failures.append(f"arguments were welded together: {separate}")

        for name, source in QUIET_CASES:
            noise = dialog_findings(source, directory) + display_findings(source)
            if noise:
                failures.append(f"false positive on {name}: {noise}")

    # Rule 4: a parked entry whose literal is gone must be reported, not
    # silently ignored. This is the failure the rule was written for — the
    # lists all shrink by the string being *fixed*, which is exactly when the
    # entry stops matching and nothing says so. On its first run it found a
    # real one, ("WriteDialog", "Dialog") in the sibling checker.
    matched = ("KNOWN_GAPS", ("modules/x/x.cpp", "still here"))
    # Its report goes to stdout by design; swallow it so this test's own
    # result is the only thing the run prints.
    with contextlib.redirect_stdout(io.StringIO()):
        stale = checker.report_stale_entries({matched},
                                             list(checker.SHARED_ROOTS))
    if stale == 0:
        failures.append("rule 4 missed: every parked entry unmatched")

    # ...and must stay quiet when every entry matched, or it is unusable.
    every = {(name, key) for name, entries in (
        ("ALLOWED_UNTRANSLATED", checker.ALLOWED_UNTRANSLATED),
        ("KNOWN_GAPS", checker.KNOWN_GAPS),
        ("ALLOWED_CYRILLIC", checker.ALLOWED_CYRILLIC),
        ("ALLOWED_CYRILLIC_DIRS", checker.ALLOWED_CYRILLIC_DIRS),
        ("SHARED_CYRILLIC_GAPS", checker.SHARED_CYRILLIC_GAPS),
    ) for key in entries}
    if checker.report_stale_entries(every, list(checker.SHARED_ROOTS)) != 0:
        failures.append("rule 4 false positive: all entries matched")

    # The standalone client export has no core/ or common/, so keys naming them
    # go unmatched for a reason that is not staleness and must not be reported.
    # Getting this wrong would fail the check on exactly the layout ADR 0011
    # exists to support.
    shared_only = {(name, key) for name, entries in (
        ("ALLOWED_UNTRANSLATED", checker.ALLOWED_UNTRANSLATED),
        ("KNOWN_GAPS", checker.KNOWN_GAPS),
        ("ALLOWED_CYRILLIC_DIRS", checker.ALLOWED_CYRILLIC_DIRS),
    ) for key in entries}
    shared_only |= {("ALLOWED_CYRILLIC", key) for key in checker.ALLOWED_CYRILLIC
                    if key[0].split("/", 1)[0] not in checker.SHARED_ROOTS}
    if checker.report_stale_entries(shared_only, []) != 0:
        failures.append("rule 4 reported shared-root keys with no shared roots")

    total = len(DIALOG_SINK_CASES) + len(DISPLAY_CASES) + len(QUIET_CASES) + 5
    if failures:
        print(f"{len(failures)} of {total} case(s) failed:\n")
        for failure in failures:
            print(f"  {failure}")
        return 1

    print(f"OK: {total} checker behaviour case(s) pass.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
