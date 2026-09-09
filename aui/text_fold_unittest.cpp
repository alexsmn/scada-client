#include "aui/text_fold.h"

#include <gtest/gtest.h>

namespace {

// Moved here from main_window/command_match_unittest.cpp with the function
// itself: the Ctrl-K palette was its first caller, and the Settings surface's
// search box is its second.
TEST(FoldForSearchTest, LowersAsciiAndCyrillic) {
  EXPECT_EQ(FoldForSearch(u"Save AS"), u"save as");
  EXPECT_EQ(FoldForSearch(u"Температура"), u"температура");
  EXPECT_EQ(FoldForSearch(u"ЁЛКА"), u"ёлка");
}

// Folding is script-aware rather than byte-wise, so a string carrying both
// alphabets is folded in both. `Ё` is the case that a naive `+0x20` over the
// Cyrillic block gets wrong: it sits outside А-Я, above it in the code chart
// rather than inside it.
TEST(FoldForSearchTest, FoldsMixedScriptsAndLeavesEverythingElseAlone) {
  EXPECT_EQ(FoldForSearch(u"Modus Топология"), u"modus топология");
  EXPECT_EQ(FoldForSearch(u"Ёж"), u"ёж");
  EXPECT_EQ(FoldForSearch(u"1-2 · %"), u"1-2 · %");
  EXPECT_EQ(FoldForSearch(u""), u"");
}

// Display order is case-blind in both alphabets and puts Ё where the Russian
// alphabet does — between Е and Ж — rather than where the code chart does.
// Each pair below is one the old code-point comparison got backwards.
TEST(CompareForDisplayTest, OrdersCaseBlindAcrossBothAlphabets) {
  EXPECT_LT(CompareForDisplay(u"apple", u"Banana"), 0);
  EXPECT_LT(CompareForDisplay(u"Apple", u"banana"), 0);
  EXPECT_GT(CompareForDisplay(u"banana", u"Apple"), 0);
  EXPECT_LT(CompareForDisplay(u"аврора", u"Байкал"), 0);
  EXPECT_GT(CompareForDisplay(u"Байкал", u"аврора"), 0);
}

TEST(CompareForDisplayTest, PutsYoBetweenYeAndZhe) {
  EXPECT_LT(CompareForDisplay(u"Ель", u"Ёлка"), 0);
  EXPECT_LT(CompareForDisplay(u"Ёлка", u"Жук"), 0);
  EXPECT_LT(CompareForDisplay(u"Ёлка", u"Яблоко"), 0);
  EXPECT_LT(CompareForDisplay(u"ёлка", u"Яблоко"), 0);
  EXPECT_GT(CompareForDisplay(u"Яблоко", u"ёлка"), 0);
}

// Strings equal under the fold are still ordered, by their raw code points,
// so a sort stays deterministic and never treats "abc" and "ABC" as one.
TEST(CompareForDisplayTest, BreaksFoldTiesByCodePoint) {
  EXPECT_EQ(CompareForDisplay(u"abc", u"abc"), 0);
  EXPECT_GT(CompareForDisplay(u"abc", u"ABC"), 0);
  EXPECT_LT(CompareForDisplay(u"ABC", u"abc"), 0);
  EXPECT_LT(CompareForDisplay(u"ab", u"abc"), 0);
  EXPECT_LT(CompareForDisplay(u"", u"a"), 0);
}

}  // namespace
