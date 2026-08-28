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
