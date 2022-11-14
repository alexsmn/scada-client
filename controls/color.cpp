#include "controls/color.h"

#include "base/containers/span.h"
#include "base/string_piece_util.h"
#include "base/string_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"

#include <cassert>
#include <string.h>

namespace aui {

struct ColorEntry {
  std::u16string_view name;
  Rgba color;
};

constexpr ColorEntry kColorEntries[] = {
    {u"Прозрачный", ColorCode::Transparent},
    {u"Черный", ColorCode::Black},
    {u"Белый", ColorCode::White},
    {u"Красный", ColorCode::Red},
    {u"Зеленый", ColorCode::Green},
    {u"Синий", ColorCode::Blue},
    {u"Желтый", ColorCode::Yellow},
    {u"Голубой", ColorCode::Cyan},
    {u"Малиновый", ColorCode::Crimson},
};

static const ColorEntry* GetColorEntry(int index) {
  if (index >= 0 && index < static_cast<int>(std::size(kColorEntries)))
    return &kColorEntries[index];
  return NULL;
}

size_t GetColorCount() {
  return std::size(kColorEntries);
}

Color GetColor(int index) {
  const ColorEntry* entry = GetColorEntry(index);
  return entry ? entry->color : ColorCode::Black;
}

int FindColor(Color color) {
  for (size_t i = 0; i < std::size(kColorEntries); i++) {
    if (color == static_cast<Color>(kColorEntries[i].color))
      return i;
  }
  return -1;
}

std::u16string_view GetColorName(int index) {
  const ColorEntry* entry = GetColorEntry(index);
  return entry ? entry->name : std::u16string_view{};
}

int FindColorName(std::u16string_view str) {
  for (size_t i = 0; i < std::size(kColorEntries); i++) {
    if (IsEqualNoCase(str, kColorEntries[i].name)) {
      return i;
    }
  }
  return -1;
}

unsigned EncodeColor(Color color) {
  const Rgba& rgba = color.rgba();
  return (static_cast<unsigned>(rgba.a) << 24) |
         (static_cast<unsigned>(rgba.r) << 16) |
         (static_cast<unsigned>(rgba.g) << 8) |
         (static_cast<unsigned>(rgba.b) << 0);
}

Color DecodeColor(unsigned value) {
  return Rgba{static_cast<std::uint8_t>((value >> 16) & 0xFF),
              static_cast<std::uint8_t>((value >> 8) & 0xFF),
              static_cast<std::uint8_t>((value >> 0) & 0xFF),
              static_cast<std::uint8_t>((value >> 24) & 0xFF)};
}

Color StringToColor(std::string_view str) {
  if (!str.empty() && str[0] == '#') {
    auto hex_string = str.substr(1);
    unsigned color = 0;
    if (!base::HexStringToUInt(AsStringPiece(hex_string), &color))
      return ColorCode::Black;
    return DecodeColor(color);
  }

  return ColorCode::Black;
}

std::string ColorToString(Color color) {
  return base::StringPrintf("#%08X", EncodeColor(color));
}

}  // namespace aui
