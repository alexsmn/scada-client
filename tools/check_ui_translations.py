#!/usr/bin/env python3
"""Checks that every string in a Qt Designer form has a translation that ships.

The failure this exists to prevent: a `.ui` string's translation drifts into the
wrong context (or gets marked `vanished`) and silently stops shipping, so the
dialog renders in English while the `.ts` still *looks* translated. That is
exactly what happened to the CSV export dialog — twelve strings sat under the
empty context as `vanished`, which `lrelease` drops, and nobody noticed because
the translations were visibly there in the file.

How it checks:

  * `lupdate` extracts the strings, so the expected set and its contexts come
    from Qt's own tooling rather than from this script re-implementing `uic`.
  * A translation counts only if it would actually reach the `.qm`: present in
    the matching context, non-empty, and not `vanished`, `obsolete` or
    `unfinished` — all of which `lrelease` drops.

Scope is deliberately `.ui` files only. Most client UI strings go through the
custom `Translate()` helper, which `lupdate` cannot see; running it over the
whole tree would report thousands of false positives and, worse, tempt someone
into a full `lupdate` refresh that would mark every one of those strings
`vanished`. See KNOWN_GAPS for strings that are legitimately absent.

Usage:
    python3 client/tools/check_ui_translations.py [--client-dir DIR]
"""

import argparse
import html
import pathlib
import re
import shutil
import subprocess
import sys
import tempfile

# Strings that are deliberately not translated, with the reason. A string may
# only be listed here because translating it would be *wrong* — not because
# nobody has got round to it; use KNOWN_GAPS for that.
ALLOWED_UNTRANSLATED = {
    ("CsvExportDialog", ","): "Literal delimiter character; CsvExportDialog::"
                              "accept() parses the item as a single character.",
    ("CsvExportDialog", '"'): "Literal quote character; parsed as a single "
                              "character, same as the delimiter above.",
    # Design-time placeholder text, overwritten before the form is shown —
    # verified against the code that fills each one, not assumed from the name.
    ("WriteDialog", "Dialog"): "Default form title; WriteDialog's constructor "
                               "calls setWindowTitle() from the model.",
    ("WriteDialog", "(Description)"): "Filled by ui.descriptionLabel->setText().",
    ("WriteDialog", "(Value)"): "Filled by ui.currentValueLabel->setText().",
    ("WriteDialog", "(Condition)"): "Filled by ui.conditionLabel->setText().",
    ("WriteDialog", "(Status)"): "Filled by ui.statusLabel->setText().",
    ("WriteDialog", "units"): "Filled by ui.unitLabel->setText() from the "
                              "node's engineering units.",
    ("LimitDialog", "(Description)"): "Filled by ui.descriptionLabel->setText().",
}

# Strings that *should* be translated but are not yet. This list must only ever
# shrink: it records pre-existing gaps so the check can be enforced today,
# without pretending they are fine. Adding to it is a review conversation, not
# a fix.
KNOWN_GAPS = {
    ("LoginDialog", "Security:"),
    ("LoginDialog", "Certificate:"),
    ("LoginDialog", "Private key:"),
    ("LoginDialog", "Client certificate (.pem)"),
    ("LoginDialog", "Client private key (.pem)"),
    ("LoginDialog", "…"),
}


def parse_ts(path, active_only):
    """Maps context -> set of source strings in a .ts file.

    With active_only, keeps only entries that survive into the .qm.
    """
    result = {}
    text = path.read_text(encoding="utf-8")
    for context in re.finditer(r"<context>\s*<name>(.*?)</name>(.*?)</context>",
                               text, re.S):
        name = context.group(1)
        for message in re.finditer(r"<message[^>]*>(.*?)</message>",
                                   context.group(2), re.S):
            body = message.group(1)
            source = re.search(r"<source>(.*?)</source>", body, re.S)
            if not source:
                continue
            # .ts stores text XML-escaped; compare (and list below) real text.
            source_text = html.unescape(source.group(1))
            if active_only:
                translation = re.search(r"<translation([^>]*)>(.*?)</translation>",
                                        body, re.S)
                if not translation:
                    continue
                attributes, value = translation.group(1), translation.group(2)
                dropped = ("vanished" in attributes or "obsolete" in attributes
                           or "unfinished" in attributes)
                if dropped or not value.strip():
                    continue
            result.setdefault(name, set()).add(source_text)
    return result


def extract_ui_strings(ui_files, lupdate):
    """Runs lupdate over `ui_files` and returns context -> set of sources."""
    with tempfile.TemporaryDirectory() as directory:
        scratch = pathlib.Path(directory) / "ui_strings.ts"
        completed = subprocess.run(
            [lupdate, *[str(path) for path in ui_files], "-ts", str(scratch)],
            capture_output=True, text=True)
        if completed.returncode != 0 or not scratch.exists():
            print("lupdate failed:\n" + completed.stdout + completed.stderr,
                  file=sys.stderr)
            sys.exit(2)
        return parse_ts(scratch, active_only=False)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--client-dir", type=pathlib.Path,
                        default=pathlib.Path(__file__).resolve().parent.parent)
    args = parser.parse_args()

    lupdate = shutil.which("lupdate")
    if not lupdate:
        # Matches the build, which also degrades gracefully without Qt
        # LinguistTools rather than failing.
        print("lupdate not found; skipping the .ui translation check.")
        return 0

    client_dir = args.client_dir
    ui_files = sorted(path for path in client_dir.rglob("*.ui")
                      if "build" not in path.parts)
    ts_files = sorted(path for path in client_dir.rglob("*.ts")
                      if "build" not in path.parts)
    if not ui_files or not ts_files:
        print(f"No .ui ({len(ui_files)}) or .ts ({len(ts_files)}) files under "
              f"{client_dir}", file=sys.stderr)
        return 2

    expected = extract_ui_strings(ui_files, lupdate)

    shipped = {}
    for path in ts_files:
        for context, sources in parse_ts(path, active_only=True).items():
            shipped.setdefault(context, set()).update(sources)

    missing = []
    for context, sources in sorted(expected.items()):
        for source in sorted(sources):
            key = (context, source)
            if source in shipped.get(context, set()):
                continue
            if key in ALLOWED_UNTRANSLATED or key in KNOWN_GAPS:
                continue
            missing.append(key)

    total = sum(len(sources) for sources in expected.values())
    print(f"Checked {total} string(s) from {len(ui_files)} .ui file(s) against "
          f"{len(ts_files)} .ts file(s).")

    if KNOWN_GAPS:
        print(f"{len(KNOWN_GAPS)} known gap(s) still awaiting translation "
              f"(see KNOWN_GAPS in {pathlib.Path(__file__).name}):")
        for context, source in sorted(KNOWN_GAPS):
            print(f"  {context}: {source!r}")

    if not missing:
        print("OK: every other .ui string has a translation that ships.")
        return 0

    print(f"\n{len(missing)} .ui string(s) have no translation that would reach "
          f"the .qm:", file=sys.stderr)
    for context, source in missing:
        print(f"  {context}: {source!r}", file=sys.stderr)
    print("\nA translation must be in the *same context* the form uses "
          "(uic emits QCoreApplication::translate(\"<FormClass>\", ...)), and "
          "must not be marked vanished/obsolete/unfinished — lrelease drops "
          "those, so the dialog renders in English while the .ts still looks "
          "translated.", file=sys.stderr)
    return 1


if __name__ == "__main__":
    sys.exit(main())
