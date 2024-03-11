#include "aui/translation.h"

#include "base/strings/utf_string_conversions.h"

std::u16string Translate(std::string_view text) {
  return base::UTF8ToUTF16(text);
}
