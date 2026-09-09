#pragma once

#include <string>
#include <string_view>

// Case-folds text for searching, over the two alphabets the client's UI is
// written in.
//
// It lives in aui rather than beside either of its callers because both the
// Ctrl-K command palette and the Settings surface's search box are asking the
// same question -- "does what the operator typed appear in what they can read?"
// -- and two implementations of it would eventually answer differently for the
// same string. Deliberately not `std::tolower`: that is locale-dependent and
// byte-oriented, and neither property is wanted here.
std::u16string FoldForSearch(std::u16string_view text);

// Orders two display strings the way an operator reads a list: case-blind
// across both alphabets, with Ё/ё sorted between Е and Ж where the Russian
// alphabet puts it rather than where the code chart does (U+0401 sits above
// А, U+0451 below я). Returns <0, 0, >0 like std::u16string::compare.
//
// Every user-facing sort in the client goes through this — the Explorer's
// siblings, every sortable aui table column, the property tree, the named
// device lists — so that the collation is decided once. Until 2026-09-09 each
// of those sites compared code points, so every uppercase name sorted before
// every lowercase one and «Ёлка» sorted after «Яблоко» (task 716).
//
// Strings that fold to the same key are ordered by their raw code points, so
// distinct strings never compare equal and a sort stays deterministic.
//
// Deliberately not QCollator: aui is Qt-free below its `qt/` layer, the
// comparator has to be unit-testable without a QCoreApplication, and it must
// answer the same way on every platform — QCollator defers to ICU on one and
// to the C library on another. The cost is that it knows only the two
// alphabets FoldForSearch does, which are the two the client's UI is written
// in.
int CompareForDisplay(std::u16string_view a, std::u16string_view b);
