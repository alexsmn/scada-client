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

}  // namespace
