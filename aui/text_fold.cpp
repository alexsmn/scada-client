#include "aui/text_fold.h"

#include <algorithm>
#include <cstdint>

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

namespace {

// The collation weight of one folded character. Weights are code points
// doubled, which leaves an odd slot after every letter for a character that
// the alphabet orders differently from the code chart: ё takes the slot after
// е, so it sorts between е and ж.
uint32_t DisplayWeight(char16_t folded) {
  if (folded == 0x0451)                            // ё
    return static_cast<uint32_t>(0x0435) * 2 + 1;  // just after е
  return static_cast<uint32_t>(folded) * 2;
}

}  // namespace

int CompareForDisplay(std::u16string_view a, std::u16string_view b) {
  const std::u16string folded_a = FoldForSearch(a);
  const std::u16string folded_b = FoldForSearch(b);

  const size_t common = std::min(folded_a.size(), folded_b.size());
  for (size_t i = 0; i < common; ++i) {
    const uint32_t wa = DisplayWeight(folded_a[i]);
    const uint32_t wb = DisplayWeight(folded_b[i]);
    if (wa != wb)
      return wa < wb ? -1 : 1;
  }
  if (folded_a.size() != folded_b.size())
    return folded_a.size() < folded_b.size() ? -1 : 1;

  // Same key: fall back to the raw code points so "abc" and "ABC" keep a
  // stable relative order rather than comparing equal.
  return a.compare(b);
}
