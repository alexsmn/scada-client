#!/usr/bin/env python3
"""Behaviour tests for rule 7 of `check_ui_translations.py`.

Rule 7 resolves a `Translate(x.label)` call back to the table the member is
read from, so its failure mode is the one every checker in this directory
has: a pattern that stops matching does not fail, it reports OK over a tree
full of the defect. Rule 7 has more of that surface than its siblings, because
it has to recognise the four ways this tree spells a table — a C array, a
`std::array` with its extra brace layer, a `std::vector` built inside a
function, and a lone `constexpr` object — in both the designated and the
positional initialiser form.

The negatives matter as much: the rule reaches across translation units, so an
over-broad match sweeps in unrelated structs. Two pin that. `label` is declared
by seventeen structs in this tree and only eight of them hold English sources.

Source-only and dependency-free, like the checker it covers. Run it directly or
through ctest as `client_ui_translation_check_test`.
"""

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

import check_ui_translations as checker  # noqa: E402

# The painter, in every case below: it is what makes `label` a translated
# field at all, and leaving it out of a case is what makes the case negative.
PAINTER = 'void Paint(const Row& row){ Draw(Tr(row.label)); }'

# (name, sources, expected source strings). `sources` is {filename: text} the
# way the checker holds the tree, so a case can put the table and the painter
# in different files — which is the whole point of the rule.
CASES = (
    (
        "designated C array, painter in another file",
        {
            "row.h": 'struct Row { unsigned id; const char* label; };',
            "table.cpp": 'constexpr Row kRows[] = {{.id = 1, .label = "Users"}};',
            "panel.cpp": PAINTER,
        },
        {"Users"},
    ),
    (
        "positional aggregate shorter than the struct (trailing default)",
        {
            "row.h": 'struct Row { int id; std::string_view browse; '
                     'std::string_view label; int shape = 0; };',
            "table.cpp": 'constexpr std::array<Row, 1> kRows{{'
                         '{kId, "BrowseName", "Round-trip"}}};',
            "panel.cpp": 'void P(const Row& r){ Draw(Tr(r.label)); }',
        },
        {"Round-trip"},
    ),
    (
        "a lone constexpr object, not a table",
        {
            "row.h": 'struct Row { int id; std::string_view browse; '
                     'std::string_view label; };',
            "table.cpp": 'constexpr Row kReconnect{kId, "Reconnect", '
                         '"Reconnect now"};',
            "panel.cpp": 'void P(const Row& r){ Draw(Tr(r.label)); }',
        },
        {"Reconnect now"},
    ),
    (
        "a std::vector built inside a function, with no type at the brace",
        {
            "row.h": 'struct Row { const char* label; bool required; };',
            "table.cpp": 'std::vector<Row> RowsFor(Options o) {\n'
                         '  return {\n'
                         '      {"Upper-case letter", Has(o, kUpper)},\n'
                         '      {"Digit", Has(o, kDigit)},\n'
                         '  };\n}',
            "panel.cpp": PAINTER,
        },
        {"Upper-case letter", "Digit"},
    ),
    (
        "adjacent literals are one source string, as clang-format leaves them",
        {
            "row.h": 'struct Row { const char* label; };',
            "table.cpp": 'constexpr Row kRows[] = {{.label = "Your account "\n'
                         '                                   "cannot do this"}};',
            "panel.cpp": PAINTER,
        },
        {"Your account cannot do this"},
    ),
    # --- negatives -----------------------------------------------------------
    (
        "an already-translated QString member is not a source",
        {
            "row.h": 'struct Row { QString label; };',
            "table.cpp": 'const Row kRows[] = {{.label = QStringLiteral("x")}};',
            "panel.cpp": PAINTER,
        },
        set(),
    ),
    (
        "a same-named field in another struct is read too, by design",
        {
            "row.h": 'struct Row { const char* label; };',
            "other.h": 'struct MatrixRow { const char* label; };',
            "table.cpp": 'constexpr Row kRows[] = {{.label = "Users"}};\n'
                         'constexpr MatrixRow kMatrix[] = {{.label = "6, 33  history"}};',
            "panel.cpp": PAINTER,
        },
        # A call site names a member, never a type, so the rule cannot tell
        # `Row` from `MatrixRow` and errs toward reporting. The next case pins
        # the way out.
        {"Users", "6, 33  history"},
    ),
    (
        "the struct's own definition is not read as an initialiser",
        {
            "row.h": 'struct Row { const char* label = "unset"; };',
            "panel.cpp": PAINTER,
        },
        set(),
    ),
)


def main():
    failures = []

    for name, files, expected in CASES:
        sources = {pathlib.Path(n): text for n, text in files.items()}
        found = {row[4] for row in checker.find_table_sources(sources)}
        if found != expected:
            failures.append(f"{name}: found {sorted(found)}, "
                            f"expected {sorted(expected)}")

    # ...and NON_DISPLAY_STRUCTS is that way out. Pinned because a checker
    # whose escape hatch does not work is one somebody switches off entirely.
    sources = {pathlib.Path("row.h"): 'struct MatrixRow { const char* label; };',
               pathlib.Path("t.cpp"):
                   'constexpr MatrixRow kM[] = {{.label = "6, 33  history"}};',
               pathlib.Path("panel.cpp"): PAINTER}
    checker.NON_DISPLAY_STRUCTS["MatrixRow"] = "test fixture"
    try:
        found = {row[4] for row in checker.find_table_sources(sources)}
    finally:
        del checker.NON_DISPLAY_STRUCTS["MatrixRow"]
    if found:
        failures.append(f"NON_DISPLAY_STRUCTS did not exclude the struct: {found}")

    # The blind-spot half. A member reaching Translate() that resolves to no
    # source-string field means the table is somewhere this cannot read, and
    # saying so is the point: silence there is exactly how task 466 shipped.
    sources = {pathlib.Path("panel.cpp"):
               'void P(const Row& r){ Draw(Tr(r.caption)); }'}
    _, unresolved = checker.find_translated_fields(sources)
    if unresolved != ["caption"]:
        failures.append(f"unresolved member not reported: {unresolved}")

    # ...and must stay quiet once the field is declared as a source string,
    # or every call site in the tree reports.
    sources[pathlib.Path("row.h")] = 'struct Row { const char* caption; };'
    _, unresolved = checker.find_translated_fields(sources)
    if unresolved:
        failures.append(f"resolved member still reported: {unresolved}")

    total = len(CASES) + 3
    if failures:
        print(f"{len(failures)} of {total} case(s) failed:\n")
        for failure in failures:
            print(f"  {failure}")
        return 1

    print(f"OK: {total} rule 7 behaviour case(s) pass.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
