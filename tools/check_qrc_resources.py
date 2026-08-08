#!/usr/bin/env python3
r"""Checks that Qt resource paths named in code are shipped by `res/client.qrc`.

The failure this exists to prevent is silent. A path that names an asset the
resource file does not list produces a null `QIcon` — no exception, no log, no
build error. The row simply renders blank, and nothing notices until somebody
looks at that part of the UI.

That is not hypothetical. The favourites tree's conversion off the magenta-keyed
`wintypes.bmp` strip added `res/icons/table.svg` and `res/icons/chart-spline.svg`
and mapped them from `resources/icon_strips.h`, but edited the qrc only to
*remove* the retired bitmap. The two new glyphs were never registered, so the
favourites tree rendered no icons at all until a follow-up commit noticed and
added them.

`ConfigurationTreeGlyphs.EveryMappedGlyphIsShipped` closes this for the two
glyph tables in `resources/icon_strips.h`. It is a good test and stays; this
check generalises it to every resource path in the tree, and runs without a
build so it also covers `aui/`, which must not take on a client dependency for
a test (see `docs/client/aui-extraction.md`).

Three rules run over the tree.

**Rule 1 — every resource literal in the sources is registered.** The one that
catches the bug above.

**Rule 2 — every `<file>` entry in the qrc exists on disk.** `rcc` fails loudly
on this, so it is not silent, but it costs nothing to catch it here first —
before a build, and with a message that names the qrc line rather than a
generated-source path.

**Rule 3 — every asset under `res/` is listed in the qrc.** The other half of
rule 1: an icon that is added to the tree but never registered ships nothing,
and stays invisible until something maps it. Catching it at the point the file
lands is cheaper than catching it at the point a row goes blank.

Known limit: a path assembled at runtime (`":/icons/" + name`) cannot be
resolved by a source scanner, so rule 1 only sees complete literals — one with
a filename and an extension. There are none of the assembled form today; if one
appears, back it with a table and a unit test the way `icon_strips.h` does.

Usage:
    python3 client/tools/check_qrc_resources.py [--client-dir DIR]
"""

import argparse
import pathlib
import re
import sys
import xml.etree.ElementTree as ElementTree

# Reuse the comment-aware literal scanner rather than hand-rolling a second
# one. Telling a literal from a comment is the part that is easy to get subtly
# wrong, and that scanner already carries the reasoning for how it does it.
from check_untranslated_ui_strings import iter_literals

# Files under `res/` that are deliberately not in the qrc, with the reason.
# Rule 3 only. A file may be listed here because embedding it would be *wrong*
# — not because registering it is outstanding work.
NOT_SHIPPED_IN_QRC = {
    # The resource file itself.
    "client.qrc": "the resource file",
    # The macOS bundle icon. CMake hands it to the bundle via
    # MACOSX_BUNDLE_ICON_FILE; it is read by the OS from the .app, not by Qt
    # from the binary, so embedding it would only duplicate it.
    "client.icns": "macOS bundle icon, installed by CMake",
    # Lucide's ISC licence text. It must travel with copies of the icon files,
    # which tracking it in the repo satisfies; the running client has no use
    # for it (NOTICE is what records the obligation).
    "icons/LICENSE": "licence text, not an asset the client draws",
}

# Resource paths named in code that intentionally do not resolve, with the
# reason. Same bar as NOT_SHIPPED_IN_QRC: only when *not* shipping is correct.
ALLOWED_UNREGISTERED = {
    # Nothing yet. The deliberately-absent paths that exist today
    # (":/does/not/exist.svg" in image_util_unittest.cpp, which asserts the
    # null-icon fallback) live in test files, which are not scanned.
}

# Test and mock files. A test may name a resource that is *meant* to be absent
# — that is how the null-icon fallback is asserted — so a finding there says
# nothing about what the client ships.
EXCLUDED_NAME_PARTS = ("_unittest.", "_mock.", "_test.")

SOURCE_SUFFIXES = (".cpp", ".h", ".cppm")

# A complete resource path: the ":/" or "qrc:/" scheme, then at least one
# segment, ending in an extension. The extension is what separates a real path
# from a prefix fragment such as ":/icons/", which is used for a starts_with
# assertion and resolves to nothing on its own.
RESOURCE_LITERAL = re.compile(r"^(?:qrc)?(:/[A-Za-z0-9_./+-]*[A-Za-z0-9_-]\.[A-Za-z0-9]+)$")


def parse_qrc(qrc_path: pathlib.Path) -> dict[str, str]:
    """Maps each registered resource path to the file it is served from.

    Both are relative to `res/`: ":/icons/table.svg" -> "icons/table.svg". The
    two differ whenever a `<file>` carries an `alias`, which is why the mapping
    is kept rather than a bare set.
    """
    registered: dict[str, str] = {}
    for resource in ElementTree.parse(qrc_path).getroot().iter("qresource"):
        prefix = (resource.get("prefix") or "/").strip("/")
        for entry in resource.iter("file"):
            source = (entry.text or "").strip()
            if not source:
                continue
            name = (entry.get("alias") or source).strip("/")
            path = f":/{prefix}/{name}" if prefix else f":/{name}"
            registered[path] = source
    return registered


def iter_sources(client_dir: pathlib.Path):
    """Yields every client source file whose resource literals are meaningful."""
    for path in sorted(client_dir.rglob("*")):
        if path.suffix not in SOURCE_SUFFIXES:
            continue
        if any(part in path.name for part in EXCLUDED_NAME_PARTS):
            continue
        if "build" in path.relative_to(client_dir).parts[:-1]:
            continue
        yield path


def iter_res_assets(res_dir: pathlib.Path):
    """Yields every file under `res/`, as a path relative to it."""
    for path in sorted(res_dir.rglob("*")):
        if path.is_file():
            yield path.relative_to(res_dir).as_posix()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--client-dir", default=str(pathlib.Path(__file__).resolve().parents[1])
    )
    args = parser.parse_args()

    client_dir = pathlib.Path(args.client_dir).resolve()
    res_dir = client_dir / "res"
    qrc_path = res_dir / "client.qrc"
    if not qrc_path.is_file():
        print(f"skipped: {qrc_path} not found")
        return 0

    registered = parse_qrc(qrc_path)

    # Rule 1: every resource path named in the sources is registered.
    unregistered, allowed, scanned, literals = [], 0, 0, 0
    for path in iter_sources(client_dir):
        scanned += 1
        rel = path.relative_to(client_dir).as_posix()
        for line, raw in iter_literals(path.read_text("utf-8", "replace")):
            match = RESOURCE_LITERAL.match(raw)
            if not match:
                continue
            literals += 1
            resource = match.group(1)
            if resource in registered:
                continue
            if (rel, resource) in ALLOWED_UNREGISTERED:
                allowed += 1
            else:
                unregistered.append((rel, line, resource))

    # Rule 2: every registered entry exists on disk.
    missing = [
        (resource, source)
        for resource, source in sorted(registered.items())
        if not (res_dir / source).is_file()
    ]

    # Rule 3: every asset under res/ is registered.
    served = set(registered.values())
    orphans = [
        asset
        for asset in iter_res_assets(res_dir)
        if asset not in served and asset not in NOT_SHIPPED_IN_QRC
    ]

    if unregistered:
        print(f"{len(unregistered)} resource path(s) are not shipped:\n")
        for rel, line, resource in unregistered:
            print(f'  {rel}:{line}: "{resource}"')
        print(
            "\nA path the qrc does not list yields a null QIcon and a blank row,\n"
            "with nothing failing and nothing logged. Add a <file> entry to\n"
            "res/client.qrc (keep the icons alphabetical), or fix the typo. If a\n"
            "path is meant not to resolve, add it to ALLOWED_UNREGISTERED with\n"
            "the reason."
        )
        return 1

    if missing:
        print(f"{len(missing)} qrc entry/entries name a file that is gone:\n")
        for resource, source in missing:
            print(f"  res/client.qrc: {source}  (serves {resource})")
        print(
            "\nrcc fails the build on this. Either restore res/<file> or drop the\n"
            "entry — and if the asset is retired, check that nothing still maps\n"
            "its resource path."
        )
        return 1

    if orphans:
        print(f"{len(orphans)} asset(s) under res/ are not in the qrc:\n")
        for asset in orphans:
            print(f"  res/{asset}")
        print(
            "\nAn unregistered asset ships nothing: whatever maps it later gets a\n"
            "null icon and a blank row. Add a <file> entry to res/client.qrc, or\n"
            "— if the file is deliberately not embedded — add it to\n"
            "NOT_SHIPPED_IN_QRC with the reason."
        )
        return 1

    print(
        f"OK: every resource path resolves ({scanned} file(s) scanned, "
        f"{literals} literal(s); {len(registered)} qrc entry/entries, "
        f"{len(NOT_SHIPPED_IN_QRC)} file(s) deliberately not embedded, "
        f"{allowed} allowed unregistered path(s))."
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
