#include "main_window/command_match.h"

#include <algorithm>
#include <tuple>

std::u16string FoldForSearch(std::u16string_view text) {
  std::u16string folded;
  folded.reserve(text.size());
  for (char16_t c : text) {
    if (c >= u'A' && c <= u'Z')
      c = static_cast<char16_t>(c + (u'a' - u'A'));
    else if (c >= 0x0410 && c <= 0x042F)  // Cyrillic А-Я -> а-я
      c = static_cast<char16_t>(c + 0x20);
    else if (c == 0x0401)  // Cyrillic Ё -> ё
      c = 0x0451;
    folded.push_back(c);
  }
  return folded;
}

std::vector<CommandEntry> RankCommandMatches(
    std::span<const CommandEntry> entries,
    std::u16string_view query) {
  const std::u16string folded_query = FoldForSearch(query);

  // A candidate carries its sort key alongside the entry so the comparison is
  // computed once per entry rather than on every comparator call.
  struct Ranked {
    const CommandEntry* entry;
    bool interior;  // false = prefix match (ranks first)
    size_t position;
    size_t length;
    std::u16string folded_title;
  };

  std::vector<Ranked> ranked;
  ranked.reserve(entries.size());
  for (const CommandEntry& entry : entries) {
    std::u16string folded_title = FoldForSearch(entry.title);
    size_t position = 0;
    if (!folded_query.empty()) {
      position = folded_title.find(folded_query);
      if (position == std::u16string::npos)
        continue;
    }
    // With no query every entry ties on prefix/position, so fall straight to
    // alphabetical (length zeroed) rather than surfacing short commands first.
    const size_t length = folded_query.empty() ? 0 : entry.title.size();
    ranked.push_back(
        {&entry, position != 0, position, length, std::move(folded_title)});
  }

  std::stable_sort(
      ranked.begin(), ranked.end(), [](const Ranked& a, const Ranked& b) {
        return std::tie(a.interior, a.position, a.length, a.folded_title) <
               std::tie(b.interior, b.position, b.length, b.folded_title);
      });

  std::vector<CommandEntry> result;
  result.reserve(ranked.size());
  for (const Ranked& r : ranked)
    result.push_back(*r.entry);
  return result;
}
