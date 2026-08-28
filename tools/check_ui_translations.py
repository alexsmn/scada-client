#!/usr/bin/env python3
"""Checks that translations in the client's .ts catalogs actually ship.

Ten independent rules run. They are documented below in the order they were
written rather than by number, because each was added for a defect the ones
before it could not see, and that order is the argument for having them all.

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
`vanished`. See UI_FORM_KNOWN_GAPS for strings that are legitimately absent.

**Rule 3 — every `Translate("...")` literal has a translation that ships.**

`Translate()` looks up by source in the *empty* context and falls back to the
source when the lookup misses, so a call site whose string was never added to
`client_ru.ts` renders English in the Russian client and nothing says so —
`lupdate` cannot see `Translate()`, and rule 1 only reads `.ui` forms. This is
the third of three ways the same defect hides, and the one that catches a
newly translated call site drifting away from its catalog entry: rename the
source in the code and the entry stops matching, silently. TRANSLATE_GAPS
parks the call sites that were already in that state.

**Rule 4 — every `.ts` file under the client is one that reaches the `.qm`.**

The other three rules read *every* `.ts` file they can find, so a catalog that
nothing compiles has its translations counted as shipping — a translation that
is present, correct and unreachable, invisible to the check written to find
exactly that. Three files were in that state until 2026-08-23; the transport
dialog's eleven labels among them, rendering English inside the Russian client
while the `.ts` sat there looking translated. `qt_add_translation()` in
`app/qt/CMakeLists.txt` is the authority for what ships.

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

**Rule 10 — every message in a shipping catalog reaches the `.qm`.**

Rule 2's complement, in the same file. Rule 2 asks which of two active copies
of a source wins; this asks whether a message ships at all. `lrelease` drops
`vanished`, `obsolete`, `unfinished` and empty translations, and says so on
every build — "Ignored 6 untranslated source text(s)" was in the client's
build output from before 2026-08-16 until 2026-08-27, and no check read it.
All six turned out to be design-time `.ui` placeholder text that `uic`
overwrites before the form is shown, so the repair was `notr="true"` on the
property rather than a translation. The dead copies are the other half: twelve
`vanished` duplicates of live `CsvExportDialog` strings made lrelease's
duplicate warning permanent noise, which is what stops anyone reading the line
that mattered.

This reads the `.ts` rather than lrelease's stdout — the same answer, with no
dependency on LinguistTools being present or on the wording of its warnings.

**Rule 6 — every `tr("...")` in a `.cpp` has a translation that ships.**

The gap between rules 1 and 3: rule 1 reads `.ui` forms, rule 3 reads
`Translate()` literals, and a plain `tr()` in C++ is neither. The context is the
enclosing Q_OBJECT class, so a message filed under the wrong name ships and is
never found — `tr()` falls back to the English source exactly as a missing entry
would. That is what made the main window's title render "Page 1 (Server: ...)"
inside a Russian client: its catalog said `MainWindowQt`, after the file, while
the class is `MainWindow`. lupdate is asked for the context rather than the rule
being re-derived here.

**Rule 7 — every table string reaching `Translate()` has one that ships.**

Rule 3 can only read a literal argument, and a display string is routinely not
one: a `constexpr` table holds the strings and the painter passes a member —
`Tr(field.label)` — from another translation unit entirely. The sibling
checker cannot see the table either, because its literal-table rule fires only
in a file that draws text and a registry of display strings draws nothing. So
the strings are translatable, are never translated, and both rules pass. That
is how eleven English strings shipped inside the Russian device-diagnostics
panel (task 466) — found by rendering the panel, which is not a check.

This rule resolves the member back to the table: a field reaching `Translate()`
that a struct declares as `const char*` or `std::string_view` is an English
source, and every literal a table puts in it must have a shipping entry. A
member that resolves to no such field is reported rather than skipped — being
unable to read the source is the defect, not an exemption from it.

**Rule 8 — every string a *function* returns into `Translate()` has one
that ships.**

The sibling of rule 7, one indirection over. Rule 7 resolves `Tr(field.label)`
back to the table the member is initialised from; this resolves
`Tr(WriteBlockText(block))` back to the `switch` the function returns from.
Both are display strings that reach `Translate()` without ever appearing as a
literal at the call site, so rule 3 cannot see either, and the sibling
checker's literal-table rule fires only in a file that draws text — which the
translation unit holding the `switch` routinely is not.

Nine call sites in this tree are of this shape, reaching seven functions whose
literals sit in `switch` statements in seven other files. All 24 strings ship,
so this rule was written over a net that was already whole; it exists so the
next such function is covered when it is written rather than after it renders
English. Task 469 recorded six of the seven — `UserRoleKey`
(`main_window/status_bar/user_status_provider.cpp`) was found by the rule
itself, which is the argument for having it.

A function whose definition this cannot find, or that returns `QString` rather
than a source, is reported for the same reason rule 7 reports an unresolved
member: being unable to read the source is the defect, not an exemption.

**Rule 9 — every catalog copy is driven by the `.qm`, not by the link.**

Every rule above asks whether a translation is *in* the catalog. This asks
whether the catalog reaches the place the running app reads. On macOS that is
`client.app/Contents/MacOS/translations`, and the copy into it was an
`add_custom_command(TARGET client_qt POST_BUILD ...)` until 2026-08-27 —
POST_BUILD fires only when the target relinks, and editing `client_ru.ts`
relinks nothing, so the bundle kept the previous catalog while the `.ts`, the
`.qm` and the staged copy were all correct. Measured 2026-08-23: 55318 bytes
in the bundle against a fresh 55458. Rule 4 is the nearest neighbour and asks
the opposite question — which catalogs get compiled, not where they are put
afterwards.

**Rule 5 — every parked entry still matches something in the tree.**

UI_FORM_ALLOWED_UNTRANSLATED, UI_FORM_KNOWN_GAPS and TRANSLATE_GAPS are
keyed on strings expected to be *found*, and all three may only shrink. Fix
the string and the entry stops matching in silence, so the list overstates the
debt while covering none of the code that replaced it. The sibling rule in
`check_untranslated_ui_strings.py` guards that file's five lists.

**A note on the names.** The two lists here are keyed on
`(ui_context, source_string)` — the pair lupdate extracts from a `.ui` form —
and carry the `UI_FORM_` prefix to say so. `check_untranslated_ui_strings.py`
sits in the same directory and holds lists for the same purpose keyed on
`(source_path, literal_text)`, prefixed `LITERAL_` there. Until 2026-08-23
both pairs were spelled `ALLOWED_UNTRANSLATED` and `KNOWN_GAPS`, so a grep for
either name returned two dicts with incompatible contents and nothing said
which file an entry belonged in — a mistake that parks a key where it matches
nothing, which both files' stale-entry rules now report as a hard failure.

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
# nobody has got round to it; use UI_FORM_KNOWN_GAPS for that.
UI_FORM_ALLOWED_UNTRANSLATED = {
    ("CsvExportDialog", ","): "Literal delimiter character; CsvExportDialog::"
                              "accept() parses the item as a single character.",
    ("CsvExportDialog", '"'): "Literal quote character; parsed as a single "
                              "character, same as the delimiter above.",
    # Design-time placeholder text used to be parked here — the write and limit
    # dialogs' "(Description)", "(Value)", "(Condition)", "(Status)" and
    # "units", each overwritten by a setText() before the form is shown. They
    # were removed on 2026-08-27 in favour of `notr="true"` on the property in
    # the .ui itself, which is Qt's own way of saying a form string is not for
    # translation: lupdate then never extracts them, so there is nothing left
    # to park, nothing for lrelease to report as untranslated, and no second
    # place where the fact has to stay true. A parked entry only ever said the
    # same thing one file further from the form.
    ("LoginDialog", "\u2026"): "Ellipsis glyph on the certificate/private-key "
                               "browse buttons; the same in every language.",
}

# Strings that *should* be translated but are not yet. This list must only ever
# shrink: it records pre-existing gaps so the check can be enforced today,
# without pretending they are fine. Adding to it is a review conversation, not
# a fix.
UI_FORM_KNOWN_GAPS = {
    # Empty, and worth keeping that way: the login dialog's TLS strings were the
    # last entries here.
}


# A `Translate("literal")` call. Only a literal argument is checkable: a call
# taking a variable is resolved at runtime and says nothing about the catalog.
#
# Adjacent literals are one string in C++ and must be joined here, because
# clang-format splits any sentence long enough to matter across lines — a
# pattern that only matched a single literal would quietly stop seeing exactly
# the strings most likely to be missing.
TRANSLATE_CALL = re.compile(
    r'Translate\(\s*((?:"(?:[^"\\]|\\.)*"\s*)+)\)')
STRING_PIECE = re.compile(r'"((?:[^"\\]|\\.)*)"')
C_ESCAPES = {'\\n': '\n', '\\t': '\t', '\\"': '"', "\\'": "'",
             '\\\\': '\\'}


def literal_text(argument):
    """Joins the adjacent string literals in `argument` into their C++ value."""
    joined = "".join(STRING_PIECE.findall(argument))
    return re.sub(r'\\[nt"\'\\\\]', lambda m: C_ESCAPES[m.group(0)], joined)


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


# Rule 3's gap list: `Translate("...")` call sites with no shipping entry in
# the empty context, so they render their English source inside the Russian
# client. Same bar as the lists above — this must only ever shrink, and an
# addition is a review conversation.
#
# Empty, and it took a drain to get here: twenty-two entries were parked when
# the rule was written on 2026-08-22, fourteen of them the graph component's
# setup menu — one omission rather than fourteen decisions, which is why they
# went in a single pass (task 442).
TRANSLATE_GAPS = {
}


def find_translate_literals(cpp_files):
    """Maps a `Translate("...")` source string -> the files that call it."""
    calls = {}
    for path in cpp_files:
        text = path.read_text(encoding="utf-8", errors="replace")
        for match in TRANSLATE_CALL.finditer(text):
            calls.setdefault(literal_text(match.group(1)), set()).add(path)
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



# Rule 7's gap list: table strings that reach Translate() through a struct
# member and have no shipping entry. Same bar as every list above — it must
# only ever shrink, and an addition is a review conversation. Rule 5 does not
# read it: its keys are source strings, and a source that stops appearing in
# any table is caught by this rule's own count changing.
TABLE_TRANSLATE_GAPS = {
    # Empty. The rule found four on its first run — the page-icon labels
    # "Substation", "Device log" and "Report", and "Devices", which the rail
    # and the icon picker both draw — and they were translated in the same
    # change rather than parked.
}


# --- Rule 7 -----------------------------------------------------------------
#
# A `Translate()` call whose argument is a struct member — `Tr(field.label)` —
# is not a literal, so rule 3 cannot see the string it will look up. The
# literals live in a *table*, routinely in another translation unit, and the
# sibling checker cannot see them either: its literal-table rule fires only in
# a file that draws text, and a registry of display strings usually draws
# nothing. Eleven English strings shipped inside the Russian device-diagnostics
# panel through that seam (task 466). Neither rule was wrong on its own; the
# gap was between them.
#
# This rule closes it from the catalog side. It resolves the member back to the
# table:
#
#   1. Collect the field names that reach Translate()/Tr() as members.
#   2. Keep the ones a struct declares as a *source*-string type — `const char*`
#      or `std::string_view`. A `QString`/`std::u16string` member holds already
#      translated output, not a source, so it is not a catalog question. A name
#      that resolves to no such field means the table is somewhere this cannot
#      read, and that is reported rather than passed over: being unable to see
#      the source is the whole defect.
#   3. Read the aggregate initialisers of those structs and require every
#      literal in a translated field to have a shipping entry, exactly as rule
#      3 requires it of a literal call site.
#
# The shape to look for, in a review as much as here, is a `constexpr` table of
# display strings whose painter lives in another file.

# `Tr(x.label)` / `Translate(p->section_label)` — the member forms rule 3 skips.
TRANSLATE_MEMBER = re.compile(
    r"\b(?:Translate|Tr)\(\s*\w+\s*(?:\.|->)\s*(\w+)\s*\)")

STRUCT_DEFINITION = re.compile(r"\bstruct\s+(\w+)\s*\{")
CPP_COMMENT = re.compile(r"//[^\n]*|/\*.*?\*/", re.S)
DESIGNATOR = re.compile(r"^\s*\.\s*(\w+)\s*=(.*)$", re.S)
LONE_LITERAL = re.compile(r'^\s*((?:"(?:[^"\\]|\\.)*"\s*)+)$')

# The types that hold an English *source*. Spelled without whitespace, because
# that is how `struct_fields` normalises what it reads.
SOURCE_STRING_TYPES = ("constchar*", "charconst*", "std::string_view")

# Structs whose same-named field is not operator text.
#
# A call site names a member, not a type — `Tr(row.label)` says nothing about
# which struct `row` is — so this rule keys on the field *name*, and any struct
# declaring a source-string field of that name is read as a display table.
# Seventeen structs in this tree declare `label`; the eight that hold English
# sources are the ones this is meant to cover, and the over-reach is deliberate
# because it errs toward reporting. Where it is wrong, name the struct here
# with the reason rather than parking its strings in TABLE_TRANSLATE_GAPS,
# which is for a string that *should* be translated and is not.
NON_DISPLAY_STRUCTS = {
    # Empty. Nothing in the tree has needed it yet: every struct the rule
    # currently reaches holds strings an operator reads.
}


def balanced_braces(text, open_index):
    """The text inside the `{...}` starting at `open_index`, and its `}` index.

    String literals are skipped rather than scanned, so a brace inside one
    cannot unbalance the walk — `"{"` appears in this tree's format strings.
    """
    depth, i, n = 0, open_index, len(text)
    while i < n:
        c = text[i]
        if c == '"':
            i += 1
            while i < n and text[i] != '"':
                i += 2 if text[i] == "\\" else 1
        elif c == "{":
            depth += 1
        elif c == "}":
            depth -= 1
            if depth == 0:
                return text[open_index + 1:i], i
        i += 1
    return None, n


def split_commas(text):
    """`text` split on its top-level commas, brackets and strings respected."""
    parts, depth, current, i, n = [], 0, [], 0, len(text)
    while i < n:
        c = text[i]
        if c == '"':
            end = i + 1
            while end < n and text[end] != '"':
                end += 2 if text[end] == "\\" else 1
            current.append(text[i:end + 1])
            i = end + 1
            continue
        if c in "({[":
            depth += 1
        elif c in ")}]":
            depth -= 1
        if c == "," and depth == 0:
            parts.append("".join(current))
            current = []
        else:
            current.append(c)
        i += 1
    if "".join(current).strip():
        parts.append("".join(current))
    return parts


def struct_fields(body):
    """[(name, type)] for the data members declared in a struct body.

    Member functions are skipped by the parenthesis in their declaration, and
    the type is whitespace-stripped so `const char *` and `const char*` are one
    spelling. Nested types are not descended into: a table's element struct is
    declared at file scope in this tree, and guessing at nesting would cost
    more precision than it buys.
    """
    fields = []
    depth, current = 0, []
    for ch in CPP_COMMENT.sub(" ", body):
        if ch in "({[":
            depth += 1
        elif ch in ")}]":
            depth -= 1
        if ch == ";" and depth == 0:
            declaration = "".join(current).strip()
            current = []
            if not declaration or "(" in declaration:
                continue
            declaration = declaration.split("=")[0].strip()
            name = re.search(r"(\w+)$", declaration)
            if name:
                fields.append((name.group(1),
                               "".join(declaration[:name.start()].split())))
        else:
            current.append(ch)
    return fields


def find_translated_fields(sources):
    """Maps struct name -> (field order, the fields that reach Translate()).

    `sources` is {path: text}. Returns only the structs that have at least one
    translated field, plus the set of member names that resolved to none — the
    blind spot this rule reports rather than skips.
    """
    members = set()
    structs = {}
    for text in sources.values():
        members |= set(TRANSLATE_MEMBER.findall(text))
        for match in STRUCT_DEFINITION.finditer(text):
            body, _ = balanced_braces(text, match.end() - 1)
            if body is not None:
                structs.setdefault(match.group(1), struct_fields(body))

    targets, resolved = {}, set()
    for name, fields in structs.items():
        translated = [] if name in NON_DISPLAY_STRUCTS else [
            field for field, type_name in fields
            if field in members and type_name in SOURCE_STRING_TYPES]
        if translated:
            targets[name] = ([field for field, _ in fields], translated)
            resolved |= set(translated)
    return targets, sorted(members - resolved)


def aggregate_initialisers(region):
    """Every `{...}` nested in `region`, as (body, line offset).

    Both levels are yielded, and the caller filters by shape: a table is
    `{{...}, {...}}` under `std::array`'s extra layer or a bare `[] = {...}`,
    and a lone object is the region itself. Yielding all of them and letting
    the struct's own field list decide is what makes one walk cover the four
    spellings this tree uses.
    """
    found, i, n = [], 0, len(region)
    while i < n:
        c = region[i]
        if c == '"':
            i += 1
            while i < n and region[i] != '"':
                i += 2 if region[i] == "\\" else 1
        elif c == "{":
            body, end = balanced_braces(region, i)
            if body is not None:
                line = region[:i].count("\n")
                found.append((body, line))
                found += [(nested, line + offset)
                          for nested, offset in aggregate_initialisers(body)]
                i = end
        i += 1
    return found


def read_aggregate(body, order):
    """Maps field -> initialiser text for one aggregate, None if not one.

    Handles the designated form (`.label = "..."`) and the positional one. A
    positional aggregate may be shorter than the struct when trailing members
    carry defaults, which `ProtocolField` does; it may never be longer.
    """
    pieces = split_commas(body)
    if not pieces or len(pieces) > len(order):
        return None
    designated = [DESIGNATOR.match(piece) for piece in pieces]
    if all(designated):
        named = {match.group(1): match.group(2) for match in designated}
        return named if set(named) <= set(order) else None
    if any(designated):
        return None
    return {order[index]: piece for index, piece in enumerate(pieces)}


def find_table_sources(sources):
    """Yields (path, line, struct, field, source) for every table literal.

    Anchored on the type name followed by a brace — which covers the four ways
    this tree spells such a table: a C array, a `std::array`, a `std::vector`
    built in a function, and a lone `constexpr` object. Anchoring on the *type*
    rather than on the initialiser's shape is what keeps this precise: a bare
    `{"...", flag}` is indistinguishable from any other two-member aggregate in
    the tree, and matching on shape alone reported unrelated structs that
    happen to have a `label`.
    """
    targets, _ = find_translated_fields(sources)
    for path, text in sources.items():
        for name, (order, translated) in targets.items():
            if name not in text:
                continue
            for match in re.finditer(r"\b" + name + r"\b[^;{}]*\{", text):
                before = text[max(0, match.start() - 8):match.start()]
                if re.search(r"\b(struct|class|union)\s*$", before):
                    continue
                region, _ = balanced_braces(text, match.end() - 1)
                if region is None:
                    continue
                line = text[:match.end()].count("\n") + 1
                candidates = aggregate_initialisers(region) or [(region, 0)]
                for body, offset in candidates:
                    named = read_aggregate(body, order)
                    if named is None:
                        continue
                    for field in translated:
                        literal = LONE_LITERAL.match(named.get(field) or "")
                        if literal:
                            yield (path, line + offset, name, field,
                                   literal_text(literal.group(1)))


def report_table_translations(cpp_files, ts_files, client_dir):
    """Rule 7. Returns the number of findings."""
    sources = {}
    for path in client_dir.rglob("*"):
        if path.suffix not in (".cpp", ".h") or "build" in path.parts:
            continue
        excluded = ("_unittest.", "_test.", "_mock.")
        if any(part in path.name for part in excluded):
            continue
        sources[path] = path.read_text(encoding="utf-8", errors="replace")

    shipped = set()
    for path in ts_files:
        shipped |= parse_ts(path, active_only=True).get("", set())

    targets, unresolved = find_translated_fields(sources)
    defined = {match.group(1) for text in sources.values()
               for match in STRUCT_DEFINITION.finditer(text)}
    # Same discipline as rule 5: an exclusion naming a struct the tree no
    # longer defines excludes nothing, in silence, while reading as covered.
    unknown = sorted(name for name in NON_DISPLAY_STRUCTS
                     if name not in defined)
    rows = sorted(set(find_table_sources(sources)))
    missing = [row for row in rows
               if row[4] not in shipped and row[4] not in TABLE_TRANSLATE_GAPS]

    print(f"Checked {len(rows)} table literal(s) reaching Translate() from "
          f"{len(targets)} struct(s); {len(TABLE_TRANSLATE_GAPS)} known "
          f"gap(s).")

    if unknown:
        print(f"\n{len(unknown)} NON_DISPLAY_STRUCTS entr(y/ies) name a struct "
              f"this tree does not define:", file=sys.stderr)
        for name in unknown:
            print(f"  {name}", file=sys.stderr)
        print("\nThe exclusion covers nothing. Delete it, or re-key it against "
              "the struct that replaced it.", file=sys.stderr)

    if unresolved:
        print(f"\n{len(unresolved)} Translate() member(s) resolve to no "
              f"source-string field, so their table cannot be read:",
              file=sys.stderr)
        for field in unresolved:
            print(f"  .{field}", file=sys.stderr)
        print("\nDeclare the member as `const char*` or `std::string_view` in "
              "a struct this can find, or the strings it carries are outside "
              "every translation check — which is exactly how task 466's "
              "eleven English strings shipped.", file=sys.stderr)

    if missing:
        print(f"\n{len(missing)} table string(s) reach Translate() with no "
              f"translation that would ship, so they render English:",
              file=sys.stderr)
        for path, line, struct, field, source in missing:
            where = path.relative_to(client_dir).as_posix()
            print(f"  {source!r}  ({where}:{line}, {struct}.{field})",
                  file=sys.stderr)
        print("\nAdd each to the empty context of app/qt/client_ru.ts. The "
              "call site passes a variable, so rule 3 cannot see the string "
              "and neither can lupdate.", file=sys.stderr)

    return len(missing) + len(unresolved) + len(unknown)


# Rule 8's gap list: strings a function returns into Translate() with no
# shipping entry. Same bar as every list above — it must only ever shrink, and
# an addition is a review conversation. Rule 5 does not read it, for the reason
# TABLE_TRANSLATE_GAPS gives.
CALL_TRANSLATE_GAPS = {
    # Empty. The rule found none on its first run: all 24 strings behind the
    # tree's nine such call sites already ship. The rule was written over a
    # whole net on purpose — see its comment below.
}


# --- Rule 8 -----------------------------------------------------------------
#
# `Tr(WriteBlockText(block))` is not a literal and not a member, so neither
# rule 3 nor rule 7 can see the string it looks up. The literals sit in a
# `switch` in another translation unit, and the sibling checker cannot see them
# either: its literal-table rule fires only in a file that draws text, and the
# file holding the `switch` usually draws nothing. Rule 7 closed this seam for
# the table form; this closes it for the function form.
#
#   1. Collect the function names called as the argument of Translate()/Tr().
#   2. Find each one's *definition*, requiring a source-string return type.
#   3. Require every literal it returns to have a shipping entry, exactly as
#      rule 3 requires it of a literal call site.
#
# A name that resolves to no such definition is reported rather than skipped,
# on rule 7's principle: being unable to read the source is the whole defect.

# `Tr(WriteBlockText(block))` — the call form rules 3 and 7 both skip.
#
# Named FUNCTION_CALL rather than CALL because rule 3 already owns
# TRANSLATE_CALL, for `Translate("literal")`. Shadowing it costs nothing at
# import and silently rewrites what rule 3 matches: while this rule was being
# written, rule 3 went from 349 literals to 1 and still reported a result.
#
# A qualifier is matched and then dropped, because one call site spells its
# function `events::EventTimelineStepText` while the definition sits in that
# namespace under its bare name. Keying on the trailing identifier is what lets
# the two meet; it also means two functions of one name in different namespaces
# would be read as one, which no call site in this tree does and which would
# over-report rather than under-report if one did.
TRANSLATE_FUNCTION_CALL = re.compile(
    r"\b(?:Translate|Tr)\(\s*(?:[A-Za-z_]\w*\s*::\s*)*([A-Za-z_]\w*)\s*\(")

# A function returning an English *source*. The return type carries the meaning
# it does in rule 7: a `QString` return is already-translated output, so it is
# not a catalog question — which is also what keeps the file-local `Tr` helper
# itself, `QString Tr(std::string_view)`, from being read as a display function.
SOURCE_STRING_RETURN = re.compile(
    r"(?:std::string_view|const\s+char\s*\*|char\s+const\s*\*)\s+"
    r"([A-Za-z_]\w*)\s*\(")

# The whole return expression, so a literal reached through a conditional is
# read too: `UserSessionsLabelKey` returns `*multi ? "Multiple" : "Single"`,
# and a rule matching only `return "...";` would see neither string.
RETURN_EXPRESSION = re.compile(r"\breturn\b([^;]*);")


def balanced_parens(text, open_index):
    """The index of the `)` closing the `(` at `open_index`, or None.

    String literals are skipped rather than scanned, for the reason
    `balanced_braces` skips them: a parenthesis inside one must not unbalance
    the walk.
    """
    depth, i, n = 0, open_index, len(text)
    while i < n:
        c = text[i]
        if c == '"':
            i += 1
            while i < n and text[i] != '"':
                i += 2 if text[i] == "\\" else 1
        elif c == "(":
            depth += 1
        elif c == ")":
            depth -= 1
            if depth == 0:
                return i
        i += 1
    return None


def source_string_functions(sources):
    """Maps function name -> [(path, line, [source strings it returns])].

    Only *definitions* are read: a declaration in a header ends at its `;` and
    carries no literals, so matching it would resolve the name against an empty
    body and report the function as translated-with-nothing.
    """
    found = {}
    for path, text in sources.items():
        for match in SOURCE_STRING_RETURN.finditer(text):
            close = balanced_parens(text, match.end() - 1)
            if close is None:
                continue
            # `noexcept`, a trailing `const`, and nothing else in this tree sit
            # between the parameter list and the body; a `;` first means this
            # was a declaration.
            tail = text[close + 1:close + 40]
            brace = tail.find("{")
            if brace < 0 or ";" in tail[:brace]:
                continue
            body, _ = balanced_braces(text, close + 1 + brace)
            if body is None:
                continue
            # Comments are dropped from the body only, so a commented-out
            # `return "..."` is not read as a shipping source. The reported
            # line comes from the definition, so the offsets this shifts do
            # not matter.
            body = CPP_COMMENT.sub(" ", body)
            literals = []
            for statement in RETURN_EXPRESSION.finditer(body):
                for piece in STRING_PIECE.finditer(statement.group(1)):
                    literals.append(literal_text(piece.group(0)))
            line = text[:match.start()].count("\n") + 1
            found.setdefault(match.group(1), []).append((path, line, literals))
    return found


def find_call_sources(sources):
    """(rows, unresolved) for every literal a function returns into Translate().

    A row is (path, line, function, source). `unresolved` holds the names
    called inside Translate() that no readable definition returns a source
    string for — reported rather than skipped, on rule 7's principle.
    """
    called = set()
    for text in sources.values():
        called |= set(TRANSLATE_FUNCTION_CALL.findall(text))

    definitions = source_string_functions(sources)
    rows, unresolved = [], []
    for name in sorted(called):
        if name not in definitions:
            unresolved.append(name)
            continue
        for path, line, literals in definitions[name]:
            for source in literals:
                rows.append((path, line, name, source))
    return sorted(set(rows)), unresolved


def report_call_translations(ts_files, client_dir):
    """Rule 8. Returns the number of findings."""
    sources = {}
    for path in client_dir.rglob("*"):
        if path.suffix not in (".cpp", ".h") or "build" in path.parts:
            continue
        excluded = ("_unittest.", "_test.", "_mock.")
        if any(part in path.name for part in excluded):
            continue
        sources[path] = path.read_text(encoding="utf-8", errors="replace")

    shipped = set()
    for path in ts_files:
        shipped |= parse_ts(path, active_only=True).get("", set())

    rows, unresolved = find_call_sources(sources)
    # An empty source is `return {};`'s neighbour in a switch that covers every
    # enumerator -- unreachable text rather than an untranslated string.
    missing = [row for row in rows if row[3]
               and row[3] not in shipped
               and row[3] not in CALL_TRANSLATE_GAPS]

    print(f"Checked {len(rows)} string(s) returned into Translate() by "
          f"{len({row[2] for row in rows})} function(s); "
          f"{len(CALL_TRANSLATE_GAPS)} known gap(s).")

    if unresolved:
        print(f"\n{len(unresolved)} function(s) called inside Translate() have "
              f"no definition returning a source string:", file=sys.stderr)
        for name in unresolved:
            print(f"  {name}()", file=sys.stderr)
        print("\nDeclare it as returning `std::string_view` or `const char*` "
              "in a file this can read, or the strings it returns are outside "
              "every translation check — the seam task 466's eleven English "
              "strings shipped through, one indirection over.", file=sys.stderr)

    if missing:
        print(f"\n{len(missing)} string(s) returned into Translate() have no "
              f"translation that would ship, so they render English:",
              file=sys.stderr)
        for path, line, name, source in missing:
            where = path.relative_to(client_dir).as_posix()
            print(f"  {source!r}  ({where}:{line}, {name}())", file=sys.stderr)
        print("\nAdd each to the empty context of app/qt/client_ru.ts. The "
              "call site passes a call, so rule 3 cannot see the string and "
              "neither can lupdate.", file=sys.stderr)

    return len(missing) + len(unresolved)


# The one catalog that reaches the .qm. `qt_add_translation()` in
# app/qt/CMakeLists.txt compiles exactly the files named to it, so a .ts file
# it does not name is unreachable however correct its contents are.
SHIPPED_CATALOG_CMAKE = "app/qt/CMakeLists.txt"
# CMake takes an argument quoted or bare, and both forms are ordinary here, so
# match either — a check that fails on valid code is a check someone switches
# off.
QT_ADD_TRANSLATION = re.compile(
    r'qt_add_translation\(\s*\w+\s+((?:"?[^\s")]+\.ts"?\s*)+)\)')


def find_shipped_catalogs(client_dir):
    """The .ts files app/qt/CMakeLists.txt actually compiles, as paths."""
    cmake = client_dir / SHIPPED_CATALOG_CMAKE
    if not cmake.exists():
        return None
    text = cmake.read_text(encoding="utf-8")
    shipped = set()
    for call in QT_ADD_TRANSLATION.finditer(text):
        for name in re.findall(r'"?([^\s")]+\.ts)"?', call.group(1)):
            shipped.add((cmake.parent / name).resolve())
    return shipped


def report_unshipped_catalogs(ts_files, client_dir):
    """Rule 4. Returns the count of .ts files that never reach the .qm.

    Returns None — not 0 — when it could not tell, so the caller prints
    "SKIPPED" rather than this rule's OK line over a tree it never read.

    The failure this exists to prevent: a translation that is present, correct,
    and unreachable. Three catalogs sat in that state until 2026-08-23 —
    `app_ru.ts`, `main_window_ru_qt.ts` and `transport_dialog_ru.ts`, eleven of
    whose messages were the transport dialog's labels, so an operator
    configuring a serial device read them in English inside the Russian client.
    A dead lconvert merge was supposed to combine them and never ran.

    Nothing else can see this. Rules 1-3 read *every* .ts file under the client
    and so treat an orphan's translations as shipping, which is precisely how
    the gap stayed invisible to the check written to find gaps.
    """
    shipped = find_shipped_catalogs(client_dir)
    if shipped is None:
        # Returning 0 here would print this rule's OK line over a tree it never
        # looked at — the exact failure the rule exists to prevent, one level
        # up. Say it skipped instead.
        print(f"SKIPPED: {SHIPPED_CATALOG_CMAKE} not found, so which catalogs "
              f"ship is unknown.", file=sys.stderr)
        return None
    if not shipped:
        print(f"\nNo qt_add_translation() call naming a .ts file was found in "
              f"{SHIPPED_CATALOG_CMAKE}, so nothing would compile a catalog "
              f"at all.", file=sys.stderr)
        return 1

    orphans = sorted(path.relative_to(client_dir).as_posix()
                     for path in ts_files if path.resolve() not in shipped)
    if not orphans:
        return 0

    print(f"\n{len(orphans)} .ts file(s) are never compiled into the .qm, so "
          f"every translation in them renders as its English source:",
          file=sys.stderr)
    for name in orphans:
        print(f"  {name}", file=sys.stderr)
    print(f"\nMove the messages into a catalog that "
          f"{SHIPPED_CATALOG_CMAKE}'s qt_add_translation() names and delete "
          f"the orphan, or add the file to that call. Do not leave it in "
          f"place: the other rules here read every .ts file under the client "
          f"and would report its contents as shipping.", file=sys.stderr)
    return len(orphans)


# A POST_BUILD custom command attached to the client executable. POST_BUILD
# runs only when the target *relinks*, so a translation-only edit never
# reaches whatever it copies. Matched loosely — any TARGET/POST_BUILD pair in
# one add_custom_command call — because the failure is the mechanism, not a
# spelling of it.
POST_BUILD_COMMAND = re.compile(
    r'add_custom_command\s*\(\s*TARGET\s+\w+[^)]*?\bPOST_BUILD\b',
    re.DOTALL)
# The catalogs those commands are forbidden to carry.
CATALOG_TOKENS = ("TRANSLATIONS_OUT", "QTBASE_RU_QM", ".qm")


def report_link_driven_catalog_copies(client_dir):
    """Rule 9. Returns the number of catalog copies driven by the link.

    The failure this exists to prevent: a catalog that is correct, compiled,
    staged — and stale wherever the running app actually reads it. The macOS
    bundle copy was an `add_custom_command(TARGET client_qt POST_BUILD ...)`
    until 2026-08-27, and POST_BUILD fires only when the target relinks. A
    `.ts` edit regenerates `client_ru.qm` and re-stages it without relinking
    anything, so `client.app/Contents/MacOS/translations` — the only path
    `InstalledTranslation` reads inside a bundle — kept whatever catalog the
    last link had left there. Measured 2026-08-23: 55318 bytes against a fresh
    55458, missing both new messages, and `touch app/qt/main.cpp` "fixed" it.

    Nothing else here can see it. Every other rule reads the `.ts` and the
    sources, and by all of them the translation is present and shipping; the
    file it is shipping *into* is the part no rule looked at. Rule 4 is the
    nearest neighbour and asks the opposite question — which catalogs the
    build compiles, not where it puts them once compiled.

    The fix is the shape `client_qt_copy_translations` already had: an
    `add_custom_command(OUTPUT ...)` with the `.qm` files in `DEPENDS`, plus a
    target, so the build system can see what the copy is actually made of.
    """
    cmake = client_dir / SHIPPED_CATALOG_CMAKE
    if not cmake.exists():
        print(f"SKIPPED: {SHIPPED_CATALOG_CMAKE} not found, so how the "
              f"catalogs are copied is unknown.", file=sys.stderr)
        return None

    text = cmake.read_text(encoding="utf-8")
    offenders = []
    for match in POST_BUILD_COMMAND.finditer(text):
        body = text[match.start():balanced_parens_end(text, match.start())]
        if any(token in body for token in CATALOG_TOKENS):
            line = text.count("\n", 0, match.start()) + 1
            offenders.append(line)

    if not offenders:
        return 0

    print(f"\n{len(offenders)} translation copy/copies in "
          f"{SHIPPED_CATALOG_CMAKE} are driven by the link rather than by the "
          f"catalogs:", file=sys.stderr)
    for line in offenders:
        print(f"  {SHIPPED_CATALOG_CMAKE}:{line}", file=sys.stderr)
    print("\nPOST_BUILD runs only when the target relinks, so a .ts-only edit "
          "leaves the copy stale and the running app reads the previous "
          "catalog. Use add_custom_command(OUTPUT ...) with the .qm files in "
          "DEPENDS plus a target, the way client_qt_copy_translations does, "
          "and add_dependencies() it onto client_qt.", file=sys.stderr)
    return len(offenders)


def balanced_parens_end(text, start):
    """Index just past the `)` closing the call whose `(` follows `start`.

    Falls back to the end of the file when the walk cannot close the call, so
    an unmatched parenthesis makes this rule over-read rather than skip: a
    missed offender is the failure it exists to prevent.
    """
    open_index = text.index("(", start)
    close_index = balanced_parens(text, open_index)
    return len(text) if close_index is None else close_index + 1


def report_stale_entries(expected, calls):
    """Rule 5. Returns the number of parked entries that match nothing.

    The sibling of rule 4 in check_untranslated_ui_strings.py, and it exists
    for the same reason: UI_FORM_ALLOWED_UNTRANSLATED, UI_FORM_KNOWN_GAPS
    and TRANSLATE_GAPS are each keyed on something expected to be *found* —
    a (context, source) pair lupdate extracts from a .ui form, or a
    Translate() literal in the tree — and each is documented as a list that
    may only shrink. Fix the string and the entry stops matching silently,
    leaving the list overstating the debt while covering none of the code that
    replaced it.

    `expected` is lupdate's extraction, so this can only judge the two .ui
    lists when lupdate ran; the caller passes None otherwise and they are
    skipped rather than reported wholesale.
    """
    stale = []
    if expected is not None:
        pairs = {(context, source)
                 for context, sources in expected.items() for source in sources}
        stale += [("UI_FORM_ALLOWED_UNTRANSLATED", key)
                  for key in UI_FORM_ALLOWED_UNTRANSLATED if key not in pairs]
        stale += [("UI_FORM_KNOWN_GAPS", key)
                  for key in UI_FORM_KNOWN_GAPS if key not in pairs]
    stale += [("TRANSLATE_GAPS", source)
              for source in TRANSLATE_GAPS if source not in calls]
    if not stale:
        return 0

    print(f"\n{len(stale)} parked entr(y/ies) match nothing in the tree:",
          file=sys.stderr)
    for name, key in stale:
        print(f"  {name}[{key!r}]", file=sys.stderr)
    print("\nEach of these lists is keyed on a string expected to be found — a "
          ".ui form's (context, source) pair, or a Translate() literal — and "
          "each must only ever shrink. An entry matching nothing has already "
          "shrunk: delete it. Until then the list claims a debt that is paid "
          "and covers none of the code that replaced the string.",
          file=sys.stderr)
    return len(stale)


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


def find_dropped_messages(path):
    """Every <message> in `path` that lrelease would not compile.

    Yields (context, source, reason). The three `type` values and an empty
    translation are the whole of what lrelease drops, which is why this reads
    the .ts rather than lrelease's stdout: the same answer, with no dependency
    on Qt LinguistTools being installed or on the wording of its warnings.
    """
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
            source_text = html.unescape(source.group(1))
            translation = re.search(
                r"<translation([^>]*)>(.*?)</translation>", body, re.S)
            if not translation:
                yield name, source_text, "no <translation> element"
                continue
            attributes, value = translation.group(1), translation.group(2)
            for state in ("vanished", "obsolete", "unfinished"):
                if state in attributes:
                    yield name, source_text, f'type="{state}"'
                    break
            else:
                if not value.strip():
                    yield name, source_text, "empty translation"


def report_dropped_messages(ts_files):
    """Rule 10. Returns the number of messages lrelease would not compile.

    The failure this exists to prevent is two-sided, and lrelease reports both
    on every build where nobody reads them. An `unfinished` or empty entry is a
    string that renders English — six sat in the catalog from before 2026-08-16
    until 2026-08-27, all of them design-time `.ui` placeholder text that
    should never have been extracted at all. A `vanished` or `obsolete` copy of
    a source that is still live is the duplicate warning: harmless in itself,
    because lrelease drops it either way, but it is indistinguishable in the
    output from the collision rule 2 exists for, so the noise is what stops
    anyone reading either.

    Rule 2 is the deliberate complement and skips exactly these entries: it
    reports two *active* messages colliding, where which one ships is file
    order rather than intent. This one reports a message that ships not at all.

    There is no parked list on purpose. The three ways out are all repairs:
    translate it, mark the form property `notr="true"` so lupdate stops
    extracting a string that was never for translation, or delete the message.
    A list here would be a fourth way that leaves the string rendering English.
    """
    dropped = []
    for path in ts_files:
        for context, source, reason in find_dropped_messages(path):
            dropped.append((path, context, source, reason))
    if not dropped:
        return 0

    print(f"\n{len(dropped)} message(s) are in a shipping catalog but would "
          f"not be compiled into the .qm:", file=sys.stderr)
    for path, context, source, reason in dropped:
        label = context or "(empty context)"
        print(f"  {path.name} [{label}] {source!r}: {reason}", file=sys.stderr)
    print("\nlrelease drops all of these and says so on every build "
          "(\"Ignored N untranslated source text(s)\", \"dropping duplicate "
          "messages\"), where nothing reads it. Translate the string, mark the "
          ".ui property notr=\"true\" if it is design-time placeholder text, or "
          "delete the message.", file=sys.stderr)
    return len(dropped)


def extract_ui_strings(sources, lupdate):
    """Runs lupdate over `sources` and returns context -> set of sources.

    Works on `.ui` forms (rule 1) and on `.cpp` (rule 6) alike — lupdate infers
    the context either way, which is the whole point of asking it rather than
    re-deriving the rule that a `tr()` context is the enclosing Q_OBJECT class.
    """
    with tempfile.TemporaryDirectory() as directory:
        scratch = pathlib.Path(directory) / "extracted.ts"
        completed = subprocess.run(
            [lupdate, *[str(path) for path in sources], "-ts", str(scratch)],
            capture_output=True, text=True)
        if completed.returncode != 0 or not scratch.exists():
            print("lupdate failed:\n" + completed.stdout + completed.stderr,
                  file=sys.stderr)
            sys.exit(2)
        return parse_ts(scratch, active_only=False)


def report_tr_gaps(cpp_files, ts_files, lupdate):
    """Rule 6. Returns the number of `tr()` strings with no shipping message.

    The blind spot the other rules leave between them: rule 1 reads `.ui` forms
    and rule 3 reads `Translate()` literals, so a plain `tr("...")` in a `.cpp`
    is seen by neither. Two defects lived there until 2026-08-23, and both were
    invisible for the same reason.

    The first is the one that motivates asking lupdate rather than matching on a
    name. `main_window_ru_qt.ts` filed its messages under context
    `MainWindowQt` — the *file's* name; the class is `MainWindow` — so the
    window title rendered "Page 1 (Server: ...)" in English inside an otherwise
    Russian client. Nobody could have noticed while that catalog shipped
    nowhere (task 384), and once it did ship, nothing checked the context.

    The second is a string that was never in any catalog: `Loading...`, in two
    unrelated classes (`MainWindow`'s macOS-only submenu placeholder and
    `ItemDelegate`'s combo box), each needing its own context.

    Note the docstring above warns that lupdate over the tree would produce
    "thousands of false positives". That is true of *refreshing* the catalog —
    it would mark every `Translate()` string vanished — but not of reading it:
    lupdate cannot see `Translate()` at all, so what it returns here is exactly
    the `tr()` calls and nothing else. Measured at 30 strings over 509 files.
    """
    expected = extract_ui_strings(cpp_files, lupdate)

    shipped = {}
    for path in ts_files:
        for context, sources in parse_ts(path, active_only=True).items():
            shipped.setdefault(context, set()).update(sources)

    missing = [(context, source)
               for context, sources in sorted(expected.items())
               for source in sorted(sources)
               if source not in shipped.get(context, set())]

    total = sum(len(sources) for sources in expected.values())
    print(f"Checked {total} tr() string(s) from {len(cpp_files)} .cpp file(s).")
    if not missing:
        return 0

    print(f"\n{len(missing)} tr() string(s) have no translation that would "
          f"reach the .qm, so they render English:", file=sys.stderr)
    for context, source in missing:
        print(f"  [{context or 'no context'}] {source!r}", file=sys.stderr)
    print("\nAdd each to app/qt/client_ru.ts under the context named above — "
          "which is the enclosing Q_OBJECT class, not the file name, and is "
          "reported here by lupdate rather than guessed. A message filed under "
          "the wrong context ships and is never found: tr() looks up by "
          "context, so it falls back to the English source exactly as a "
          "missing entry would.", file=sys.stderr)
    return len(missing)


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

    # Rule 4 first: it decides whether the files the other rules are about to
    # read are files that ship. A green run of rules 1-3 over an unreachable
    # catalog is exactly the "reports OK while blind" failure they exist to
    # prevent, so this must not sit behind them.
    unshipped = report_unshipped_catalogs(ts_files, client_dir)
    if unshipped:
        return 1
    if unshipped is not None:
        print(f"OK: all {len(ts_files)} .ts file(s) are compiled into the .qm.")

    # Rule 2 next: it needs no lupdate, so it must not sit behind the skip
    # below or it would silently stop running wherever Qt LinguistTools is
    # absent — which is the same "reports OK while blind" failure the rule
    # itself guards against.
    if report_duplicate_sources(ts_files):
        return 1
    print(f"OK: no duplicated source strings in {len(ts_files)} .ts file(s).")

    # Rule 10 beside rule 2: same file, and the complementary question. Rule 2
    # asks which of two active copies wins; this asks whether a message ships
    # at all. Needs no lupdate either.
    if report_dropped_messages(ts_files):
        return 1
    print(f"OK: every message in {len(ts_files)} .ts file(s) reaches the .qm.")

    # Rule 3, for the same reason: it reads the catalogs and the sources, and
    # needs no lupdate.
    cpp_files = sorted(path for path in client_dir.rglob("*.cpp")
                       if "build" not in path.parts)
    if report_translate_gaps(cpp_files, ts_files, client_dir):
        return 1
    print("OK: every other Translate() string has a translation that ships.")

    # Rule 9 next: it reads app/qt/CMakeLists.txt only, needs no lupdate, and
    # asks where the compiled catalogs are copied to — the one question every
    # other rule here assumes an answer to.
    link_driven = report_link_driven_catalog_copies(client_dir)
    if link_driven:
        return 1
    if link_driven is not None:
        print("OK: every catalog copy is driven by the .qm files, not by the "
              "link.")

    # Rule 7, immediately after rule 3: it answers the same question about the
    # call sites rule 3 has to skip, and needs no lupdate either.
    if report_table_translations(cpp_files, ts_files, client_dir):
        return 1
    print("OK: every table string reaching Translate() has one that ships.")

    # Rule 8, immediately after rule 7: same seam, one indirection over, and it
    # needs no lupdate either.
    if report_call_translations(ts_files, client_dir):
        return 1
    print("OK: every string returned into Translate() has one that ships.")

    calls = find_translate_literals(cpp_files)
    lupdate = shutil.which("lupdate")
    if not lupdate:
        # The .ui lists cannot be judged without lupdate's extraction, but
        # TRANSLATE_GAPS can: it is keyed on Translate() literals, which this
        # file reads itself.
        if report_stale_entries(None, calls):
            return 1
        print("OK: no stale TRANSLATE_GAPS entries.")
        # Matches the build, which also degrades gracefully without Qt
        # LinguistTools rather than failing.
        print("lupdate not found; skipping the .ui translation check.")
        return 0

    expected = extract_ui_strings(ui_files, lupdate)

    if report_stale_entries(expected, calls):
        return 1
    print("OK: every parked entry still matches something in the tree.")

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
            if key in UI_FORM_ALLOWED_UNTRANSLATED or key in UI_FORM_KNOWN_GAPS:
                continue
            missing.append(key)

    if report_tr_gaps(cpp_files, ts_files, lupdate):
        return 1
    print("OK: every tr() string has a translation that ships.")

    total = sum(len(sources) for sources in expected.values())
    print(f"Checked {total} string(s) from {len(ui_files)} .ui file(s) against "
          f"{len(ts_files)} .ts file(s).")

    if UI_FORM_KNOWN_GAPS:
        print(f"{len(UI_FORM_KNOWN_GAPS)} known gap(s) still awaiting "
              f"translation (see UI_FORM_KNOWN_GAPS in "
              f"{pathlib.Path(__file__).name}):")
        for context, source in sorted(UI_FORM_KNOWN_GAPS):
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
