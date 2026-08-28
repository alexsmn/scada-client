#include "aui/text_fold.h"

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
