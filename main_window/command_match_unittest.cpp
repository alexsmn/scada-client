#include "main_window/command_match.h"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace {

// Builds a CommandEntry list from (id, title) pairs; detail is irrelevant to
// ranking so it is left empty.
std::vector<CommandEntry> Entries(
    std::initializer_list<std::pair<unsigned, std::u16string>> items) {
  std::vector<CommandEntry> entries;
  for (const auto& [id, title] : items)
    entries.push_back({id, title, {}});
  return entries;
}

std::vector<std::u16string> Titles(const std::vector<CommandEntry>& entries) {
  std::vector<std::u16string> titles;
  for (const CommandEntry& entry : entries)
    titles.push_back(entry.title);
  return titles;
}

TEST(FoldForSearchTest, LowersAsciiAndCyrillic) {
  EXPECT_EQ(FoldForSearch(u"Save AS"), u"save as");
  EXPECT_EQ(FoldForSearch(u"Температура"), u"температура");
  EXPECT_EQ(FoldForSearch(u"ЁЛКА"), u"ёлка");
}

TEST(RankCommandMatchesTest, EmptyQueryKeepsAllAlphabetically) {
  std::vector<CommandEntry> ranked = RankCommandMatches(
      Entries({{1, u"Gamma"}, {2, u"alpha"}, {3, u"Beta"}}), u"");
  EXPECT_EQ(Titles(ranked),
            (std::vector<std::u16string>{u"alpha", u"Beta", u"Gamma"}));
}

TEST(RankCommandMatchesTest, SubstringFiltersNonMatches) {
  std::vector<CommandEntry> ranked = RankCommandMatches(
      Entries({{1, u"Save"}, {2, u"Open"}, {3, u"Save As"}}), u"sa");
  EXPECT_EQ(Titles(ranked), (std::vector<std::u16string>{u"Save", u"Save As"}));
}

TEST(RankCommandMatchesTest, PrefixRanksAboveInterior) {
  // "en" prefixes "Enable" but appears mid-word in "Open".
  std::vector<CommandEntry> ranked =
      RankCommandMatches(Entries({{1, u"Open"}, {2, u"Enable"}}), u"en");
  EXPECT_EQ(Titles(ranked), (std::vector<std::u16string>{u"Enable", u"Open"}));
}

TEST(RankCommandMatchesTest, ShorterTitleWinsAmongMatches) {
  // All three prefix-match "save"; ties break on title length, so the ordering
  // is Save (4) < "Save As" (7) < "Save All" (8).
  std::vector<CommandEntry> ranked = RankCommandMatches(
      Entries({{1, u"Save As"}, {2, u"Save"}, {3, u"Save All"}}), u"save");
  EXPECT_EQ(Titles(ranked),
            (std::vector<std::u16string>{u"Save", u"Save As", u"Save All"}));
}

TEST(RankCommandMatchesTest, MatchingIsCaseInsensitive) {
  std::vector<CommandEntry> ranked =
      RankCommandMatches(Entries({{1, u"Save"}, {2, u"Open"}}), u"SAVE");
  EXPECT_EQ(Titles(ranked), (std::vector<std::u16string>{u"Save"}));
}

TEST(RankCommandMatchesTest, MatchesCyrillicIgnoringCase) {
  std::vector<CommandEntry> ranked = RankCommandMatches(
      Entries({{1, u"Температура"}, {2, u"Давление"}}), u"ТЕМ");
  EXPECT_EQ(Titles(ranked), (std::vector<std::u16string>{u"Температура"}));
}

TEST(RankCommandMatchesTest, NoMatchReturnsEmpty) {
  std::vector<CommandEntry> ranked =
      RankCommandMatches(Entries({{1, u"Save"}, {2, u"Open"}}), u"zzz");
  EXPECT_TRUE(ranked.empty());
}

}  // namespace
