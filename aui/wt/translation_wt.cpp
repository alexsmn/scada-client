#include "aui/translation.h"

#include "base/utf_convert.h"

std::u16string Translate(std::string_view text) {
  return UtfConvert<char16_t>(text);
}
