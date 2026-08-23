#!/usr/bin/env python3
"""Checks that translations in the client's .ts catalogs actually ship.

Two independent rules run.

**Rule 1 — every string in a Qt Designer form has a translation that ships.**

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

**Rule 3 — every `Translate("...")` literal has a translation that ships.**

`Translate()` looks up by source in the *empty* context and falls back to the
source when the lookup misses, so a call site whose string was never added to
`client_ru.ts` renders English in the Russian client and nothing says so —
`lupdate` cannot see `Translate()`, and rule 1 only reads `.ui` forms. This is
the third of three ways the same defect hides, and the one that catches a
newly translated call site drifting away from its catalog entry: rename the
source in the code and the entry stops matching, silently. TRANSLATE_GAPS
parks the call sites that were already in that state.

**Rule 2 — no two shipping messages in one context share a source string.**

`lrelease` keeps one entry per (context, source) and silently drops the rest
(`Warning: dropping duplicate messages`), so for a duplicated pair one
translation never ships — and *which* one wins is a property of file order, not
of intent. The empty context is where this bites, because `Translate()` looks
up by source with no context to disambiguate with: ten sources were duplicated
there, and `New` carried both «Создание» (the CATEGORY_NEW menu group) and
«Новый» (the New-graph/page actions), so the three action call sites rendered
the group label. The fix for a genuine clash of meanings is two distinct source
strings, the way `To Favourites` was split from `Add to Favourites`; for an
accidental repeat it is deleting the later copy.

This rule needs no `lupdate`, so it runs even where Qt LinguistTools is absent.

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
    ("LoginDialog", "\u2026"): "Ellipsis glyph on the certificate/private-key "
                               "browse buttons; the same in every language.",
}

# Strings that *should* be translated but are not yet. This list must only ever
# shrink: it records pre-existing gaps so the check can be enforced today,
# without pretending they are fine. Adding to it is a review conversation, not
# a fix.
KNOWN_GAPS = {
    # Empty, and worth keeping that way: the login dialog's TLS strings were the
    # last entries here.
}


# A `Translate("literal")` call. Only a literal argument is checkable: a call
# taking a variable is resolved at runtime and says nothing about the catalog.
TRANSLATE_CALL = re.compile(r'Translate\(\s*"((?:[^"\\]|\\.)*)"\s*\)')


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


# Rule 3's KNOWN_GAPS: `Translate("...")` call sites with no shipping entry in
# the empty context, so they render their English source inside the Russian
# client. Same bar as the lists above — this must only ever shrink, and an
# addition is a review conversation. Each is mapped to one call site; several
# have more. Measured 2026-08-22: 21 of 332 Translate() literals.
#
# Two thirds are the graph component's setup menu, which suggests one omission
# rather than twenty; task 437 carries the work of draining this.
TRANSLATE_GAPS = {
    'Add Pane':
        "modules/graph/graph_component.cpp",
    'Alias':
        "modules/node_table/node_table_menu_model.cpp",
    'Background':
        "modules/graph/graph_component.cpp",
    'Background Color...':
        "modules/graph/graph_component.cpp",
    'Color':
        "modules/graph/graph_component.cpp",
    'Delete Pane':
        "modules/graph/graph_component.cpp",
    'Dots':
        "modules/graph/graph_component.cpp",
    'Failed to download file.':
        "modules/filesystem/filesystem_commands.cpp",
    'Graph Setup...':
        "modules/graph/graph_component.cpp",
    'Icon':
        "main_window/main_window_qt.cpp",
    'Legend':
        "modules/graph/graph_component.cpp",
    'Line Color...':
        "modules/graph/graph_component.cpp",
    'New page':
        "main_window/activity_bar_qt.cpp",
    'Now':
        "modules/graph/graph_component.cpp",
    'Restart the application to apply the new language now?':
        "main_window/main_window_module.cpp",
    'Scroll Bar':
        "modules/graph/graph_component.cpp",
    'Scroll to Now':
        "modules/graph/graph_component.cpp",
    'Setup':
        "modules/graph/graph_component.cpp",
    'Speech':
        "main_window/main_window_module.cpp",
    'Steps':
        "modules/graph/graph_component.cpp",
    'open in another window':
        "main_window/activity_bar_qt.cpp",
}


def find_translate_literals(cpp_files):
    """Maps a `Translate("...")` source string -> the files that call it."""
    calls = {}
    for path in cpp_files:
        text = path.read_text(encoding="utf-8", errors="replace")
        for match in TRANSLATE_CALL.finditer(text):
            calls.setdefault(match.group(1), set()).add(path)
    return calls


def report_translate_gaps(cpp_files, ts_files, client_dir):
    """Rule 3. Returns the number of call sites with no shipping translation."""
    shipped = set()
    for path in ts_files:
        # Translate() always looks up in the empty context; a form context is
        # rule 1's business and cannot satisfy a Translate() call.
        shipped |= parse_ts(path, active_only=True).get("", set())

    calls = find_translate_literals(cpp_files)
    missing = sorted(source for source in calls
                     if source not in shipped and source not in TRANSLATE_GAPS)

    print(f"Checked {len(calls)} Translate() literal(s) from "
          f"{len(cpp_files)} .cpp file(s); "
          f"{len(TRANSLATE_GAPS)} known gap(s).")
    if not missing:
        return 0

    print(f"\n{len(missing)} Translate() string(s) have no translation that "
          f"would reach the .qm, so they render English:", file=sys.stderr)
    for source in missing:
        where = sorted(path.relative_to(client_dir).as_posix()
                       for path in calls[source])[0]
        print(f"  {source!r}  ({where})", file=sys.stderr)
    print("\nAdd the message to the empty context of app/qt/client_ru.ts — "
          "Translate() looks up by source there and falls back to the English "
          "source on a miss, which is why nothing else notices.",
          file=sys.stderr)
    return len(missing)


def find_duplicate_sources(path):
    """Lists (context, source, [translations]) duplicated among shipping entries.

    Deliberately separate from `parse_ts`, which returns sets and so cannot see
    a duplicate at all. `vanished`/`obsolete` copies are skipped: `lrelease`
    drops those anyway, so they are not what collides.
    """
    duplicates = []
    text = path.read_text(encoding="utf-8")
    for context in re.finditer(r"<context>\s*<name>(.*?)</name>(.*?)</context>",
                               text, re.S):
        name = context.group(1)
        seen = {}
        for message in re.finditer(r"<message[^>]*>(.*?)</message>",
                                   context.group(2), re.S):
            body = message.group(1)
            source = re.search(r"<source>(.*?)</source>", body, re.S)
            translation = re.search(
                r"<translation([^>]*)>(.*?)</translation>", body, re.S)
            if not source or not translation:
                continue
            attributes, value = translation.group(1), translation.group(2)
            if "vanished" in attributes or "obsolete" in attributes:
                continue
            seen.setdefault(html.unescape(source.group(1)), []).append(
                html.unescape(value))
        for source, translations in seen.items():
            if len(translations) > 1:
                duplicates.append((name, source, translations))
    return duplicates


def report_duplicate_sources(ts_files):
    """Rule 2. Returns the number of colliding (context, source) pairs."""
    collisions = []
    for path in ts_files:
        for context, source, translations in find_duplicate_sources(path):
            collisions.append((path, context, source, translations))
    if not collisions:
        return 0

    print(f"\n{len(collisions)} source string(s) are duplicated inside one "
          f"context, so lrelease keeps one entry and drops the rest:",
          file=sys.stderr)
    for path, context, source, translations in collisions:
        label = context or "(empty context)"
        print(f"  {path.name} [{label}] {source!r}: "
              + ", ".join(repr(value) for value in translations),
              file=sys.stderr)
    print("\nWhich copy survives is file order, not intent. If the copies mean "
          "different things, give them distinct source strings (Translate() has "
          "no context to disambiguate with); if they are an accidental repeat, "
          "delete the later one.", file=sys.stderr)
    return len(collisions)


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

    client_dir = args.client_dir
    ui_files = sorted(path for path in client_dir.rglob("*.ui")
                      if "build" not in path.parts)
    ts_files = sorted(path for path in client_dir.rglob("*.ts")
                      if "build" not in path.parts)
    if not ui_files or not ts_files:
        print(f"No .ui ({len(ui_files)}) or .ts ({len(ts_files)}) files under "
              f"{client_dir}", file=sys.stderr)
        return 2

    # Rule 2 first: it needs no lupdate, so it must not sit behind the skip
    # below or it would silently stop running wherever Qt LinguistTools is
    # absent — which is the same "reports OK while blind" failure the rule
    # itself guards against.
    if report_duplicate_sources(ts_files):
        return 1
    print(f"OK: no duplicated source strings in {len(ts_files)} .ts file(s).")

    # Rule 3, for the same reason: it reads the catalogs and the sources, and
    # needs no lupdate.
    cpp_files = sorted(path for path in client_dir.rglob("*.cpp")
                       if "build" not in path.parts)
    if report_translate_gaps(cpp_files, ts_files, client_dir):
        return 1
    print("OK: every other Translate() string has a translation that ships.")

    lupdate = shutil.which("lupdate")
    if not lupdate:
        # Matches the build, which also degrades gracefully without Qt
        # LinguistTools rather than failing.
        print("lupdate not found; skipping the .ui translation check.")
        return 0

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
