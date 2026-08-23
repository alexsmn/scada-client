#!/usr/bin/env python3
r"""Checks that user-facing strings are not hard-coded as string literals.

The failure this exists to prevent: a string that reaches the operator is
written as a raw UTF-16 literal instead of going through `Translate()`, so it
can never be translated — and *nothing* notices. `lupdate` does not recognise
`Translate()`, so it never reports the string as missing; the `.ui` checker
only reads Designer forms; and a lookup that finds no entry silently falls back
to the English source. The result renders in English inside an otherwise
Russian client, indefinitely.

That is not hypothetical. The select-before-operate confirmation — the most
safety-critical text in the client — shipped in English because its preamble
was a bare `u"..."` literal. The configuration import/export path had sixteen
more, of which *eleven already had finished Russian translations* sitting
unused in `client_ru.ts`: the catalog entries were fine, the code simply never
asked for them.

**Every literal form counts, not just `u"..."`.** Until 2026-08-22 the
literal pattern was `u"..."` alone, which made the check blind to two forms
that ship the same defect and had each done so: a wide `L"..."` — the shape
`u16format(L"Property {} not found", ...)` uses, five call sites of it — and a
plain `"..."` handed to a Qt widget, `setWindowTitle("Graph Setup")`. Both read
as ordinary code and neither was reported. The pattern now accepts the
`u8`/`u`/`U`/`L` prefixes and none, which is what a C++ string literal actually
is.

That widening is only usable together with the next paragraph: a plain `"..."`
is also how a *correctly* translated string is spelled, since `Translate("...")`
takes one. So before a region is judged, every `Translate(...)`-family call in
it is blanked out, and only what remains is a finding. Without that step the
widened pattern reports 40 correct call sites and nothing else.

Three independent rules run over the tree.

**Rule 1 — untranslated strings at a user-facing sink.**

  * It looks only at arguments actually reaching a user-facing sink (see
    `SINKS`) — a message box, a resource error, a file-dialog title, a window
    title. Scanning every literal in the tree would drown in protocol strings,
    node-id paths and format specifiers.
  * It resolves character-array constants, because that is how the bug hides: a
    namespace-scope `const char16_t kExportTitle[] = u"Export";` *cannot* call
    `Translate()` (which reads the installed catalog and needs a running
    QApplication), so the constant form is the tempting wrong answer.
  * Adjacent literals are joined, so a wrapped `u"a " u"b"` is judged whole —
    *adjacent* meaning nothing but whitespace between them. Literals in
    different arguments are judged separately; joining those produced
    nonsense findings like `"No data to export.Export"`, a message and a
    dialog title welded together.
  * A literal that is nothing but markup or format placeholders is ignored,
    so `about_dialog.cpp`'s `"<p>&copy; %3 <a href='%4'>%5</a></p>"` is not a
    finding while `"Property {} not found"` is.

**Rule 2 — no Cyrillic in a string literal, anywhere.**

Rule 1 only sees the six sinks, so it is blind to the larger population: a
status-strip cell, a menu caption, a grid placeholder. Those never reach a
message box, so a Russian literal sits there permanently untranslatable and
nothing complains. Rule 2 closes that by inverting the test — instead of asking
"does this sink get a literal", it asks "does any literal contain Russian",
which is decidable without knowing where the string goes.

It reads *literals only*, not comments: Russian in a comment is documentation
(`command_match.cpp` documents the Cyrillic case-folding range; `watch_view.cpp`
justifies a column width by the header text "Адрес") and is entirely correct.
`\uXXXX` escapes are decoded first, since escaping is how the literals hid from
a naive grep for Cyrillic in the first place.

Rule 2 also runs over `core/` and `common/` (see `SHARED_ROOTS`), because the
client is not where operator-facing text ends. A status description, a quality
flag and a boolean label are all produced down there and rendered verbatim by
the client; before `TranslateUiText` (`core/base/ui_text.h`) existed they were
Russian literals with no seam to translate them through, which is precisely the
condition this rule detects. Those trees are optional — the client repo is
published standalone, where they are absent — so the check skips whichever it
cannot find rather than failing.

**Rule 3 — no untranslated literal at a Qt text-display API.**

Rule 1's six sinks are all *dialogs*. A string painted onto a widget reaches
the operator just as surely and passes none of them: the trend legend's column
headers rendered `Current / Min / Max / Average / @ cursor` in English inside
the Russian client for as long as that legend existed, because
`metrix_graph.cpp` contains no message box and so no rule-1 sink at all.

So rule 3 inverts the same way rule 2 did, one layer over: instead of asking
"does a dialog get a literal", it asks "does a literal reach anything that
*draws text*" — `QT_TEXT_SINKS` below. Scoped that way it is quiet: 5 findings
across the client, where the same widening applied without the scope would
report thousands.

It has a second half, for the shape that defeats every use-site rule. The
legend's headers are not passed to `drawText` — a *table* of them sits at file
scope and the paint loop indexes it (`QString::fromUtf8(column.header)`), so
no amount of reading the call site finds a literal there. `LITERAL_TABLE`
therefore also reports a file-scope `k…[] = {…}` initialiser holding
letter-bearing literals, but only in a file that draws text at all. That
conjunct is what keeps it precise: two hits tree-wide, one of them the legend.

Note that data flow any longer than that is out of reach and always will be —
a literal that reaches a widget through a function call, a member or a
container is not findable by reading source with regular expressions. Rule 3
narrows the blind spot; it does not close it.

Usage:
    python3 client/tools/check_untranslated_ui_strings.py [--client-dir DIR]
"""

import argparse
import pathlib
import re
import sys

# Calls whose string arguments are shown to the operator. Each entry is the
# spelling to search for and the opening delimiter of its argument list.
SINKS = (
    "RunMessageBox",
    # The fire-and-forget wrapper over RunMessageBox (aui/show_message_box.h),
    # for callers that are not coroutines. It is as much a message box as the
    # awaitable one, so a literal reaching it is the same defect; without this
    # entry, moving a call site onto the wrapper would silently drop it out of
    # this check.
    "ShowMessageBox",
    "ResourceError",
    "ShowResourceError",
    "SelectOpenFile",
    "SelectSaveFile",
    "setWindowTitle",
)

# Literals that are deliberately NOT translated, with the reason. A string may
# only be listed here because translating it would be *wrong* — not because
# nobody has got round to it; use KNOWN_GAPS for that.
ALLOWED_UNTRANSLATED = {
    # The data-interchange strings that must stay English
    # (export_data_writer.cpp column headers, kNodeIdTitle) are not reached by
    # any sink, so they never appear here in the first place.
    #
    # A default *filename*, not a title. It reaches SelectSaveFile through
    # `default_path`, and this check cannot tell one field of the params struct
    # from another; the file the operator saves must keep its extension and its
    # ASCII name whatever language the client is in.
    (
        "modules/export/configuration/excel_configuration_commands.cpp",
        "configuration.csv",
    ): "default filename, not a title",
}

# Strings that *should* be translated but are not yet. This list must only ever
# shrink: it records pre-existing gaps so the check can be enforced today,
# without pretending they are fine. Adding to it is a review conversation, not
# a fix.
KNOWN_GAPS = {
    # The export/import message boxes that used to be here are fixed. Every
    # entry below was invisible until 2026-08-22, when the literal pattern
    # learned the `L"..."`, plain `"..."` and raw `R"(...)"` forms and rule 3
    # started reading Qt's display calls. None of them is new breakage: each
    # has been shipping in English inside the Russian client, unreported.
    #
    # This list must only ever shrink. Task 425 drained nine of the fourteen
    # on 2026-08-22 — the u16format(L"...") family, the two window titles and
    # the tab label, the Modus placeholder, and task 351's Vidicon address —
    # each by wrapping the string in Translate() and adding the message to
    # client_ru.ts. What is left is task 418's, below, and only that.

    # --- task 418: the trend legend value grid, a file-scope table the paint
    # loop indexes. Note the source strings are short and generic, so settle
    # the catalog collision (task 152) before translating them.
    ("modules/graph/metrix_graph.cpp", "Current"): "task 418",
    ("modules/graph/metrix_graph.cpp", "Min"): "task 418",
    ("modules/graph/metrix_graph.cpp", "Max"): "task 418",
    ("modules/graph/metrix_graph.cpp", "Average"): "task 418",
    ("modules/graph/metrix_graph.cpp", "@ cursor"): "task 418",
}

# Directories whose Cyrillic literals are not UI text and must stay as they are.
# Rule 2 only; rule 1's sinks are absent from these files anyway.
ALLOWED_CYRILLIC_DIRS = {
    # External interface, not our text. These OLESTR names and state strings are
    # the Vidicon Modus 6.30 ActiveX protocol's own identifiers — renaming one
    # breaks the binding at runtime, which is why client/CLAUDE.md forbids it.
    "modules/modus/activex": "Vidicon ActiveX protocol identifiers",
}

# Individual Cyrillic literals that are not UI text, keyed by (path, decoded
# text) exactly as ALLOWED_UNTRANSLATED is. Same bar: only when translating
# would be *wrong*.
ALLOWED_CYRILLIC = {
    # Wire data, not UI text. A configuration export writes the *localized*
    # boolean label, so files exported by a Russian client — and every file
    # exported before the labels went through TranslateUiText — carry these
    # words. Import has to keep recognising them. See the comment on
    # ParseBoolLabel.
    ("common/common/format.cpp", "Да"): "legacy exported BOOL spelling",
    ("common/common/format.cpp", "Нет"): "legacy exported BOOL spelling",
    # A character class for the expression lexer, not text shown to anyone: it
    # lists the letters an identifier may contain (scada_expression.cpp).
    (
        "common/common/scada_expression.cpp",
        "абвгдеёжзийклмнопрстуфхцчшщьыъэюяАБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШ",
    ): "identifier character class",
    ("common/common/scada_expression.cpp", "ЩЬЫЪЭЮЯ"): "identifier character class",
}

# The shared libraries below the client, scanned by rule 2 only. They produce
# operator-facing text too — a status description, a quality flag, a boolean
# label — and it reaches the client through `TranslateUiText`
# (`core/base/ui_text.h`), so a Russian literal here is exactly as
# untranslatable as one in the client. They are optional: the client repo is
# published standalone, where these directories do not exist.
#
# Rule 1 does not apply — none of the six sinks exist below the UI layer.
SHARED_ROOTS = ("core", "common")

# Parts of SHARED_ROOTS not yet swept, with what each still needs decided. This
# list must only ever shrink; it is KNOWN_GAPS for rule 2.
SHARED_CYRILLIC_GAPS = {
    # Single-glyph quality modifiers rendered inline in a tree label ("[НР] …").
    # Translating them needs a catalog key per glyph, and Translate() has no
    # disambiguation context, so a one-letter source would collide with every
    # other one-letter source. Needs the glyph vocabulary decided first.
    "common/node_service/node_format.cpp": "quality-modifier glyphs",
    "common/address_space/node_format.cpp": "quality-modifier glyphs",
    # OPC UA standard node display names, served over the wire by a process
    # that installs no translator — routing them through TranslateUiText would
    # make the server send English, changing what operators see, with nothing
    # on the client side to translate it back.
    "common/address_space/standard_address_space.cpp": "wire-served display names",
    # Windows API failure text, on a path that has no translator installed.
    "core/base/win/format_hresult.cpp": "OS error fallback",
}

# Files whose strings never reach an operator.
# "build" is here because a product's build tree now lives inside its own
# source tree (ADR 0011), and vcpkg installs its headers under it — so this
# source-only check was reading Qt's own qlocale_p.h and reporting the
# Cyrillic literal in it as an untranslated client string. Under the
# superproject build the output went somewhere else entirely and nothing
# here ever saw it.
EXCLUDED_DIR_PARTS = ("build", "test", "tools")
EXCLUDED_NAME_PARTS = ("_unittest.", "_mock.", "_test.")

# Any C++ string literal, whatever its encoding prefix, raw ones included. The
# lookbehind keeps the tail of an identifier from being read as a prefix, so
# `foo"x"` (a user-defined literal or a macro paste, never a plain string) is
# not matched.
#
# The raw alternative has to come first and has to be here rather than handled
# separately: a raw string's body routinely contains quotes of its own, so a
# scanner that does not know the form reads *those* as the literals. That is
# not hypothetical — `modus_view.cpp` paints a raw-string HTML block, and the
# first version of this widening reported its two `href` values while missing
# the English paragraph they sit in.
LITERAL = re.compile(
    r'(?<![A-Za-z0-9_])(?:u8|u|U|L)?'
    r'(?:R"([^()\\\s]{0,16})\((.*?)\)\1"'
    r'|"((?:[^"\\]|\\.)*)")',
    re.S,
)


def literal_text(match: re.Match) -> str:
    """The body of a LITERAL match, raw or ordinary."""
    return match.group(2) if match.group(2) is not None else match.group(3)

# `const char16_t kFoo[] = u"...";` and its narrow/wide siblings — the form
# that cannot call Translate().
CONSTANT = re.compile(
    r"(?:const|constexpr)\s+(?:char|char8_t|char16_t|char32_t|wchar_t)\s+(\w+)"
    r'\s*\[\s*\]\s*=\s*((?:\s*(?:u8|u|U|L)?"(?:[^"\\]|\\.)*")+)\s*;'
)

# The ways this tree spells "translate me". `Translate()` is the real one;
# `Tr()` is a per-file `QString` wrapper over it that several Qt views define
# in an anonymous namespace (164 call sites), and `tr()`/`QT_TRANSLATE_NOOP()`
# are Qt's own. A literal inside any of them is correct code, and the widened
# LITERAL above would otherwise report every one of them.
#
# The lookbehind excludes a member or qualified call — `x.tr(`, `Foo::tr(` —
# from being taken as a bare `tr(`.
TRANSLATORS = ("Translate", "TranslateUiText", "Tr", "tr")

# Qt's mark-for-translation macros, as a family rather than as the one spelling
# this tree happens to use today (`QT_TRANSLATE_NOOP`, 4 sites). Naming just
# that one would repeat in miniature the bug this whole check was fixed for: a
# pattern that knows one form of a construct and reports every sibling form as
# a defect. Covers QT_TR_NOOP, QT_TR_N_NOOP, QT_TRANSLATE_NOOP,
# QT_TRANSLATE_NOOP3, QT_TRANSLATE_N_NOOP3 and the _UTF8 variants.
QT_NOOP_MACRO = r"QT_TR(?:ANSLATE)?(?:_N)?_NOOP\d?(?:_UTF8)?"

TRANSLATOR_CALL = re.compile(
    r"(?<![A-Za-z0-9_:.>])(?:"
    + "|".join(TRANSLATORS)
    + "|"
    + QT_NOOP_MACRO
    + r")\s*\("
)

HAS_LETTER = re.compile(r"[A-Za-zЀ-ӿ]")

# Markup and format placeholders. A literal made only of these carries no
# operator-readable words, so it is not a translation gap: HTML scaffolding,
# `%1`-style Qt placeholders, `{}`-style std::format ones, HTML entities.
MARKUP_ONLY = re.compile(r"<[^<>]*>|\{[^{}]*\}|%\d+|%[sdfl]|&\w+;")


def is_excluded(path: pathlib.Path, client_dir: pathlib.Path) -> bool:
    rel = path.relative_to(client_dir)
    if any(part in EXCLUDED_DIR_PARTS for part in rel.parts[:-1]):
        return True
    return any(part in path.name for part in EXCLUDED_NAME_PARTS)


def join_literals(text: str) -> str:
    """Concatenates every literal in `text`. For a single expression only."""
    return "".join(literal_text(m) for m in LITERAL.finditer(text))


def literal_groups(text: str):
    """Yields each run of *adjacent* literals in `text`, concatenated.

    The compiler joins `"a " "b"` into one string, so they must be judged as
    one — but only when nothing separates them. Two literals in different
    arguments are two strings, and welding them together produced findings
    that quoted text no operator ever sees.
    """
    group, end = "", None
    for m in LITERAL.finditer(text):
        if end is not None and text[end : m.start()].strip():
            yield group
            group = ""
        group += literal_text(m)
        end = m.end()
    if group:
        yield group


def blank_translator_calls(text: str) -> str:
    """Replaces every `Translate(...)`-family call in `text` with blanks.

    A literal inside one is correct code. Blanking rather than deleting keeps
    every remaining offset — and so every reported line number — intact.
    """
    while True:
        m = TRANSLATOR_CALL.search(text)
        if not m:
            return text
        body = argument_region(text, m.end() - 1)
        end = m.end() + len(body)
        text = text[: m.start()] + " " * (end - m.start() + 1) + text[end + 1 :]


def normalize(text: str) -> str:
    """Collapses whitespace runs, so a wrapped literal keys and prints as one.

    A raw-string HTML block carries its source indentation, which would
    otherwise make both the allow/gap key and the printed finding depend on how
    the code happens to be formatted.
    """
    return " ".join(text.split())


def is_operator_text(text: str) -> bool:
    """True when `text` carries words an operator would read."""
    return bool(text) and bool(HAS_LETTER.search(MARKUP_ONLY.sub("", text)))


def argument_region(text: str, open_index: int) -> str:
    """Returns the balanced (...) or {...} region starting at `open_index`."""
    opener = text[open_index]
    closer = {"(": ")", "{": "}"}[opener]
    depth = 0
    i = open_index
    while i < len(text):
        c = text[i]
        if c == '"':  # skip string literals so their parens do not count
            i += 1
            while i < len(text) and text[i] != '"':
                i += 2 if text[i] == "\\" else 1
        elif c == opener:
            depth += 1
        elif c == closer:
            depth -= 1
            if depth == 0:
                return text[open_index + 1 : i]
        i += 1
    return ""


def scan_file(path: pathlib.Path, client_dir: pathlib.Path):
    """Yields (line, sink, text, via_constant) for each untranslated literal."""
    source = path.read_text("utf-8", "replace")
    constants = {
        m.group(1): join_literals(m.group(2)) for m in CONSTANT.finditer(source)
    }

    for sink in SINKS:
        for m in re.finditer(r"\b" + sink + r"\b\s*(?:<[^<>]*>\s*)?([({])", source):
            region = argument_region(source, m.end() - 1)
            if not region:
                continue
            line = source[: m.start()].count("\n") + 1
            untranslated = blank_translator_calls(region)

            for text in literal_groups(untranslated):
                if is_operator_text(text):
                    yield line, sink, normalize(text), None

            # The constant form: an identifier resolving to a literal.
            for name in re.findall(r"\b(\w+)\b", untranslated):
                text = constants.get(name)
                if is_operator_text(text or ""):
                    yield line, sink, normalize(text), name


# Qt calls that put text in front of the operator without going anywhere near a
# dialog — rule 3's sinks. Deliberately a short list of calls whose *whole
# purpose* is to display words: a wider net (addItem, setData, setProperty)
# picks up model plumbing and object names and stops being readable.
QT_TEXT_SINKS = (
    "drawText",
    "setText",
    "setPlaceholderText",
    "setToolTip",
    "setStatusTip",
    "addTab",
    "setTabText",
    "setTitle",
    "setHeaderData",
)

# A file-scope `const … kName[] = { … };` initialiser. Rule 3 reports one whose
# elements are operator-readable words, because a table indexed by a paint loop
# is out of reach of every use-site rule — see the module docstring.
LITERAL_TABLE = re.compile(
    r"(?:const|constexpr)\s[\w:<>,\s*&]*?\b(k\w+)\s*\[\s*\]\s*=\s*\{"
)


def scan_file_for_display_literals(source: str):
    """Yields (line, sink, text) for each untranslated literal shown by Qt."""
    draws_text = False

    for sink in QT_TEXT_SINKS:
        for m in re.finditer(r"\b" + sink + r"\b\s*(?:<[^<>]*>\s*)?([({])", source):
            draws_text = True
            region = argument_region(source, m.end() - 1)
            if not region:
                continue
            line = source[: m.start()].count("\n") + 1
            for text in literal_groups(blank_translator_calls(region)):
                if is_operator_text(text):
                    yield line, sink, normalize(text)

    # The table form. Only in a file that draws text at all: the same
    # initialiser elsewhere is a protocol table, an id list or a lookup, and
    # reporting those would drown the rule.
    if not draws_text:
        return
    for m in LITERAL_TABLE.finditer(source):
        body = argument_region(source, m.end() - 1)
        for text in literal_groups(body):
            if is_operator_text(text):
                yield source[: m.start()].count("\n") + 1, m.group(1), normalize(text)


CYRILLIC = re.compile(r"[Ѐ-ӿ]")
# `\uXXXX`, `\xXX` and the escaped backslash, which must be consumed as one
# token so `\\u0410` is not mistaken for an escape.
ESCAPE = re.compile(r"\\(?:u([0-9A-Fa-f]{4})|x([0-9A-Fa-f]{1,4})|(.))")


def decode_escapes(text: str) -> str:
    """Resolves the escapes that can hide a Cyrillic character in a literal."""

    def one(m: re.Match) -> str:
        if m.group(1) or m.group(2):
            return chr(int(m.group(1) or m.group(2), 16))
        return m.group(3)

    return ESCAPE.sub(one, text)


def iter_literals(source: str):
    """Yields (line, text) for every string/char literal outside a comment.

    Hand-written rather than regex-driven because the two constructs this must
    tell apart — a literal and a comment — can each contain the other's opening
    delimiter, and a scanner that gets that backwards either misses real
    literals or reports Russian prose in a comment as a defect.
    """
    i, line, n = 0, 1, len(source)
    while i < n:
        c = source[i]
        if c == "\n":
            line += 1
            i += 1
        elif source.startswith("//", i):
            i = source.find("\n", i)
            if i < 0:
                return
        elif source.startswith("/*", i):
            end = source.find("*/", i + 2)
            end = n if end < 0 else end + 2
            line += source.count("\n", i, end)
            i = end
        elif c in ('"', "'"):
            start_line, start = line, i + 1
            i += 1
            while i < n and source[i] != c:
                if source[i] == "\\":
                    i += 1
                elif source[i] == "\n":  # unterminated; keep the count honest
                    line += 1
                i += 1
            yield start_line, source[start:i]
            i += 1
        else:
            i += 1


def scan_file_for_cyrillic(path: pathlib.Path, rel: str):
    """Yields (line, text) for each literal carrying Cyrillic characters."""
    for line, raw in iter_literals(path.read_text("utf-8", "replace")):
        text = decode_escapes(raw)
        if CYRILLIC.search(text):
            yield line, text


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--client-dir", default=str(pathlib.Path(__file__).resolve().parents[1]))
    args = parser.parse_args()

    client_dir = pathlib.Path(args.client_dir).resolve()
    if not (client_dir / "modules").is_dir():
        print(f"skipped: {client_dir} does not look like the client tree")
        return 0

    findings, allowed, gaps = [], 0, 0
    displayed = []
    cyrillic, cyrillic_allowed, cyrillic_gaps = [], 0, 0
    scanned = 0
    for path in sorted(client_dir.rglob("*")):
        if path.suffix not in (".cpp", ".h") or is_excluded(path, client_dir):
            continue
        scanned += 1
        rel = path.relative_to(client_dir).as_posix()
        for line, sink, text, via in scan_file(path, client_dir):
            if (rel, text) in ALLOWED_UNTRANSLATED:
                allowed += 1
            elif (rel, text) in KNOWN_GAPS:
                gaps += 1
            else:
                findings.append((rel, line, sink, text, via))

        for line, sink, text in scan_file_for_display_literals(
            path.read_text("utf-8", "replace")
        ):
            if (rel, text) in ALLOWED_UNTRANSLATED:
                allowed += 1
            elif (rel, text) in KNOWN_GAPS:
                gaps += 1
            else:
                displayed.append((rel, line, sink, text))

        directory = rel.rsplit("/", 1)[0]
        for line, text in scan_file_for_cyrillic(path, rel):
            if directory in ALLOWED_CYRILLIC_DIRS or (rel, text) in ALLOWED_CYRILLIC:
                cyrillic_allowed += 1
            else:
                cyrillic.append((rel, line, text))

    # Rule 2 over the shared libraries. Paths are reported relative to the
    # superproject (`core/…`, `common/…`) so they cannot be confused with the
    # client-relative ones above.
    shared_scanned = 0
    for root_name in SHARED_ROOTS:
        root = client_dir.parent / root_name
        if not root.is_dir():
            continue
        for path in sorted(root.rglob("*")):
            if path.suffix not in (".cpp", ".h", ".cppm") or is_excluded(path, root):
                continue
            shared_scanned += 1
            rel = f"{root_name}/{path.relative_to(root).as_posix()}"
            for line, text in scan_file_for_cyrillic(path, rel):
                if rel in SHARED_CYRILLIC_GAPS:
                    cyrillic_gaps += 1
                elif (rel, text) in ALLOWED_CYRILLIC:
                    cyrillic_allowed += 1
                else:
                    cyrillic.append((rel, line, text))
    scanned += shared_scanned

    if cyrillic:
        print(f"{len(cyrillic)} literal(s) carry Russian text:\n")
        for rel, line, text in cyrillic:
            shown = text if len(text) <= 68 else text[:65] + "..."
            print(f'  {rel}:{line}: "{shown}"')
        print(
            "\nA Russian literal can never be translated and no other check\n"
            "sees it. Replace it with Translate(\"<English source>\") and add the\n"
            "Russian to the *empty* context of app/qt/client_ru.ts by hand —\n"
            "lupdate cannot see Translate(), so it will not add the entry.\n"
            "Russian in a *comment* is fine and is not reported; if a literal is\n"
            "an external protocol identifier rather than UI text, add it to\n"
            "ALLOWED_CYRILLIC with the reason."
        )
        return 1

    if findings or displayed:
        total = len(findings) + len(displayed)
        print(f"{total} user-facing string(s) cannot be translated:\n")
        for rel, line, sink, text, via in findings:
            shown = text if len(text) <= 68 else text[:65] + "..."
            through = f" (via the constant {via})" if via else ""
            print(f"  {rel}:{line}: {sink}{through}")
            print(f'      "{shown}"')
        for rel, line, sink, text in displayed:
            shown = text if len(text) <= 68 else text[:65] + "..."
            print(f"  {rel}:{line}: {sink} (shown by Qt, rule 3)")
            print(f'      "{shown}"')
        print(
            "\nWrap the string in Translate(\"...\") and add it to the *empty*\n"
            "context of app/qt/client_ru.ts — Translate() looks up with an empty\n"
            "context, and lupdate cannot see the call, so the entry is added by\n"
            "hand. A namespace-scope character-array constant cannot call\n"
            "Translate() at all (it needs a running QApplication); make it a\n"
            "function. The encoding prefix makes no difference: L\"...\",\n"
            "u8\"...\", a raw R\"(...)\" and a bare \"...\" are all reported, because\n"
            "each has shipped this defect. If the literal is a filename, a URL\n"
            "or a protocol token rather than words an operator reads, add it to\n"
            "ALLOWED_UNTRANSLATED with the reason."
        )
        return 1

    print(
        f"OK: no untranslatable user-facing strings and no Russian literals "
        f"({scanned} file(s) scanned, {shared_scanned} of them shared; "
        f"{len(SINKS)} dialog sink(s) and {len(QT_TEXT_SINKS)} display "
        f"sink(s); {allowed} allowed, {gaps} known gap(s), "
        f"{cyrillic_allowed} allowed Cyrillic literal(s), "
        f"{cyrillic_gaps} literal(s) in not-yet-swept shared files)."
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
