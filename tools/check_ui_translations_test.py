#!/usr/bin/env python3
"""Behaviour tests for rules 3, 7, 8, 9 and 10 of `check_ui_translations.py`.

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

Rule 8 is rule 7 one indirection over — it resolves `Tr(WriteBlockText(b))`
back to the `switch` the function returns from — and it has the same failure
mode and the same need for negatives. Its own traps are the ones the tree
sprang while it was written: a namespace-qualified call site whose definition
carries no qualifier, a literal reached through a conditional rather than a
bare `return "...";`, a header declaration that must not resolve to an empty
body, and a `QString`-returning function, which is already-translated output
and not a catalog question.

Rule 9 is covered here for a different reason: it is the only rule that reads
CMake rather than C++ or XML, and the shape it forbids —
`add_custom_command(TARGET client_qt POST_BUILD ...)` copying a catalog —
looks exactly like the shape it permits until you notice which keyword drives
it. A pattern that stops matching leaves the rule green over the very defect
it was written for, and the defect is invisible everywhere else: the catalog
is compiled, staged and correct, and only the copy the running app reads is
stale.

Rule 10 is here for the same reason as rule 9 and one more: it has no parked
list, so every one of its cases is either a repair or a report, and a pattern
that stops matching leaves a catalog full of strings that render English while
the file still looks translated.

Rule 3's literal collector is covered for the reason the wrapper existed: it
matched `Translate("...")` only, while sixteen Qt panels reach the catalog
through a file-local `Tr()` of their own, so 146 call sites were unchecked and
three of them were shipping English. A pattern that stops seeing the wrapper
puts them back, silently.

Source-only and dependency-free, like the checker it covers. Run it directly or
through ctest as `client_ui_translation_check_test`.
"""

import io
import pathlib
import sys
import tempfile

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


# --- Rule 8 ------------------------------------------------------------------

# The caller, in every case below: `Tr(<call>)` is what makes the function a
# display function at all, so a case that leaves it out is negative by that.
CALLER = 'QString Row(Block b){ return Tr(WriteBlockText(b)); }'

# (name, sources, expected source strings).
CALL_CASES = (
    (
        "switch in another translation unit",
        {
            "text.cpp": 'std::string_view WriteBlockText(Block b){'
                        ' switch (b) { case Block::kNo: return "Not commandable";'
                        ' case Block::kOff: return "Offline"; } return {}; }',
            "panel.cpp": CALLER,
        },
        {"Not commandable", "Offline"},
    ),
    (
        "const char* return type",
        {
            "text.cpp": 'const char* WriteBlockText(Block b){ return "Disabled"; }',
            "panel.cpp": CALLER,
        },
        {"Disabled"},
    ),
    (
        # `events::EventTimelineStepText` is spelled qualified at its call site
        # and bare at its definition. Keying on the trailing identifier is what
        # lets the two meet; before it did, the rule silently saw nothing here.
        "namespace-qualified call site, unqualified definition",
        {
            "step.cpp": 'std::string_view WriteBlockText(Step s){ return "Raised"; }',
            "panel.cpp": 'QString R(Step s){ return Tr(events::WriteBlockText(s)); }',
        },
        {"Raised"},
    ),
    (
        # `UserSessionsLabelKey` returns `*multi ? "Multiple" : "Single"`, so a
        # rule matching only `return "...";` would see neither string.
        "both arms of a conditional return",
        {
            "text.cpp": 'const char* WriteBlockText(bool on){'
                        ' return on ? "Multiple" : "Single"; }',
            "panel.cpp": CALLER,
        },
        {"Multiple", "Single"},
    ),
    (
        # Already-translated output, not a source: the file-local
        # `QString Tr(std::string_view)` helper is itself this shape, and
        # reading it as a display function would report every call site.
        "QString return type is not a source",
        {
            "text.cpp": 'QString WriteBlockText(Block b){ return "Offline"; }',
            "panel.cpp": CALLER,
        },
        set(),
    ),
    (
        # A declaration ends at its `;` and carries no literals. Resolving the
        # name against it would report the function as translated-with-nothing,
        # which reads as covered.
        "header declaration alone resolves to nothing",
        {
            # The next `{` in the file belongs to whatever follows the
            # declaration, so a matcher that only looks for a brace attaches
            # the wrong body -- and reports its strings under this name.
            "text.h": 'std::string_view WriteBlockText(Block b);\n'
                      'QString Other(){ return "Wrong body"; }',
            "panel.cpp": CALLER,
        },
        set(),
    ),
    (
        "a commented-out return is not a shipping source",
        {
            "text.cpp": 'std::string_view WriteBlockText(Block b){'
                        ' // return "Retired";\n return "Offline"; }',
            "panel.cpp": CALLER,
        },
        {"Offline"},
    ),
    (
        # Only returned literals are display strings. A log line or a lookup
        # key in the same body is not one, and sweeping it in would park
        # non-UI strings in the catalog.
        "a literal that is not returned is not a source",
        {
            "text.cpp": 'std::string_view WriteBlockText(Block b){'
                        ' Log("write blocked"); return "Offline"; }',
            "panel.cpp": CALLER,
        },
        {"Offline"},
    ),
    (
        "no caller means no display function",
        {
            "text.cpp": 'std::string_view WriteBlockText(Block b){ return "Offline"; }',
        },
        set(),
    ),
)


# Rule 9. Each case is one app/qt/CMakeLists.txt and the number of link-driven
# catalog copies it should report.
RULE9_CASES = (
    (
        "the shipped shape: OUTPUT plus a target, catalogs in DEPENDS",
        '''qt_add_translation(TRANSLATIONS_OUT "client_ru.ts")
add_custom_command(
  OUTPUT ${BUNDLE_TRANSLATIONS}
  DEPENDS ${TRANSLATIONS_OUT} ${QTBASE_RU_QM}
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${TRANSLATIONS_OUT} ${QTBASE_RU_QM} "${BUNDLE_TRANSLATIONS_DIR}")
add_custom_target(client_qt_copy_bundle_translations
  DEPENDS ${BUNDLE_TRANSLATIONS})
''',
        0,
    ),
    (
        "the defect: POST_BUILD copying the catalogs",
        '''qt_add_translation(TRANSLATIONS_OUT "client_ru.ts")
add_custom_command(TARGET client_qt POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${TRANSLATIONS_OUT} ${QTBASE_RU_QM}
    "$<TARGET_BUNDLE_CONTENT_DIR:client_qt>/MacOS/translations")
''',
        1,
    ),
    (
        "a POST_BUILD naming a .qm by path rather than by variable",
        '''qt_add_translation(TRANSLATIONS_OUT "client_ru.ts")
add_custom_command(TARGET client_qt POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_BINARY_DIR}/client_ru.qm" "${BUNDLE}/translations")
''',
        1,
    ),
    (
        "a POST_BUILD that has nothing to do with catalogs stays allowed",
        '''qt_add_translation(TRANSLATIONS_OUT "client_ru.ts")
add_custom_command(TARGET client_qt POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${ICON}" "$<TARGET_BUNDLE_CONTENT_DIR:client_qt>/Resources")
''',
        0,
    ),
)


# Rule 10. Each case is one .ts file and the number of messages in it that
# lrelease would not compile.
RULE10_CASES = (
    (
        "a translated message ships",
        '''<TS><context><name>WriteDialog</name>
<message><source>Write value</source>
<translation>\u0417\u0430\u043f\u0438\u0441\u044c</translation></message>
</context></TS>''',
        0,
    ),
    (
        "the six that shipped English: unfinished and empty",
        '''<TS><context><name>WriteDialog</name>
<message><source>(Status)</source>
<translation type="unfinished"></translation></message>
</context></TS>''',
        1,
    ),
    (
        "an empty translation with no type attribute is dropped too",
        '''<TS><context><name>WriteDialog</name>
<message><source>units</source><translation></translation></message>
</context></TS>''',
        1,
    ),
    (
        "the vanished duplicates behind the permanent lrelease warning",
        '''<TS><context><name>CsvExportDialog</name>
<message><source>Tab</source>
<translation>\u0422\u0430\u0431</translation></message>
<message><source>Tab</source>
<translation type="vanished">\u0422\u0430\u0431</translation></message>
</context></TS>''',
        1,
    ),
    (
        "obsolete counts the same, and both contexts are reported",
        '''<TS><context><name>Dialog</name>
<message><source>OK</source>
<translation type="obsolete">\u041e\u041a</translation></message>
</context><context><name>AboutDialog</name>
<message><source>Cancel</source>
<translation type="vanished">\u041e\u0442\u043c\u0435\u043d\u0430</translation></message>
</context></TS>''',
        2,
    ),
)


# (name, source text, expected source strings) for rule 3's literal collector.
RULE3_CASES = (
    ("plain Translate",
     'auto t = Translate("Measurements");',
     {"Measurements"}),
    ("file-local Tr wrapper",
     'QString Tr(std::string_view t){ return Translate(t); }\n'
     'auto s = Tr("Current colour");',
     {"Current colour"}),
    ("adjacent literals are one string",
     'auto t = Tr("Opens the two-stage command confirm. "\n'
     '            "Actions are logged.");',
     {"Opens the two-stage command confirm. Actions are logged."}),
    # A member argument is rule 7's business and says nothing about the catalog
    # here; a longer identifier ending in `Tr` is not the wrapper at all.
    ("member argument is not a literal",
     'Draw(Tr(row.label));',
     set()),
    ("identifier ending in Tr does not match",
     'auto s = FormatStr("Not a catalog string");',
     set()),
)


def run_rule3_cases(tmp_root):
    """Returns a list of failure descriptions for RULE3_CASES."""
    failures = []
    for index, (name, text, expected) in enumerate(RULE3_CASES):
        path = tmp_root / f"case{index}.cpp"
        path.write_text(text, encoding="utf-8")
        found = set(checker.find_translate_literals([path]))
        if found != expected:
            failures.append(f"rule 3 / {name}: found {sorted(found)}, "
                            f"expected {sorted(expected)}")
    return failures


def run_rule10_cases():
    """Returns a list of failure descriptions for RULE10_CASES."""
    failures = []
    for name, ts_text, expected in RULE10_CASES:
        with tempfile.TemporaryDirectory() as temp:
            path = pathlib.Path(temp) / "client_ru.ts"
            path.write_text(ts_text, encoding="utf-8")
            stderr, sys.stderr = sys.stderr, io.StringIO()
            try:
                found = checker.report_dropped_messages([path])
            finally:
                sys.stderr = stderr
        if found != expected:
            failures.append(f"rule 10 / {name}: reported {found}, "
                            f"expected {expected}")
    return failures


def run_rule9_cases():
    """Returns a list of failure descriptions for RULE9_CASES."""
    failures = []
    for name, cmake_text, expected in RULE9_CASES:
        with tempfile.TemporaryDirectory() as temp:
            root = pathlib.Path(temp)
            cmake = root / checker.SHIPPED_CATALOG_CMAKE
            cmake.parent.mkdir(parents=True)
            cmake.write_text(cmake_text, encoding="utf-8")
            # The positive cases print the rule's own diagnosis, which is the
            # expected result here rather than output anyone should read.
            stderr, sys.stderr = sys.stderr, io.StringIO()
            try:
                found = checker.report_link_driven_catalog_copies(root)
            finally:
                sys.stderr = stderr
        if found != expected:
            failures.append(f"rule 9 / {name}: reported {found}, "
                            f"expected {expected}")
    return failures


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

    for name, files, expected in CALL_CASES:
        sources = {pathlib.Path(n): text for n, text in files.items()}
        rows, _ = checker.find_call_sources(sources)
        found = {row[3] for row in rows}
        if found != expected:
            failures.append(f"rule 8 / {name}: found {sorted(found)}, "
                            f"expected {sorted(expected)}")

    # The blind-spot half, rule 8's version. A function called inside
    # Translate() whose source this cannot read is the defect, not an
    # exemption -- the same bar rule 7 sets for an unresolved member.
    sources = {pathlib.Path("panel.cpp"):
               'QString R(Block b){ return Tr(WriteBlockText(b)); }'}
    _, unresolved = checker.find_call_sources(sources)
    if unresolved != ["WriteBlockText"]:
        failures.append(f"rule 8: unresolved call not reported: {unresolved}")

    # ...and must stay quiet once the definition is readable.
    sources[pathlib.Path("text.cpp")] = (
        'std::string_view WriteBlockText(Block b){ return "Offline"; }')
    _, unresolved = checker.find_call_sources(sources)
    if unresolved:
        failures.append(f"rule 8: resolved call still reported: {unresolved}")

    with tempfile.TemporaryDirectory() as temp:
        failures.extend(run_rule3_cases(pathlib.Path(temp)))

    failures.extend(run_rule9_cases())
    failures.extend(run_rule10_cases())

    total = (len(CASES) + len(CALL_CASES) + len(RULE3_CASES)
             + len(RULE9_CASES) + len(RULE10_CASES) + 5)
    if failures:
        print(f"{len(failures)} of {total} case(s) failed:\n")
        for failure in failures:
            print(f"  {failure}")
        return 1

    print(f"OK: {total} rule 3, 7, 8, 9 and 10 behaviour case(s) pass.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
