#include "aui/color.h"

#include "aui/translation.h"
#include "base/string_util.h"
#include <charconv>
#include <format>

#include <cassert>
#include <string.h>

namespace scada::aui {

struct ColorEntry {
  std::string_view debug_name;
  std::u16string_view name;
  Rgba color;
};

constexpr ColorEntry kColorEntries[] = {
    {"Transparent", u"Transparent", ColorCode::Transparent},
    {"Black", u"Black", ColorCode::Black},
    {"White", u"White", ColorCode::White},
    {"Red", u"Red", ColorCode::Red},
    {"Green", u"Green", ColorCode::Green},
    {"Blue", u"Blue", ColorCode::Blue},
    {"Yellow", u"Yellow", ColorCode::Yellow},
    {"Cyan", u"Cyan", ColorCode::Cyan},
    {"Crimson", u"Crimson", ColorCode::Crimson},
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

std::u16string GetColorName(int index) {
  // The palette names are English source strings; the UI language comes from
  // the translation catalog, where the names are registered as no-ops
  // (aui/qt/translation_qt.cpp). Untranslated, a Russian Formats table read
  // «Green» beside «Подпись 0».
  const ColorEntry* entry = GetColorEntry(index);
  return entry ? Translate(entry->debug_name) : std::u16string{};
}

std::string_view GetColorDebugName(int index) {
  const ColorEntry* entry = GetColorEntry(index);
  return entry ? entry->debug_name : std::string_view{};
}

int FindColorName(std::u16string_view str) {
  for (size_t i = 0; i < std::size(kColorEntries); i++) {
    // A name picked from a translated list arrives translated; compare the
    // translated name exactly (case folding is ASCII-only) and keep accepting
    // the English one.
    if (str == GetColorName(static_cast<int>(i)) ||
        IsAsciiCaseInsensitiveEqual(str, kColorEntries[i].name)) {
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
    auto [ptr, ec] = std::from_chars(hex_string.data(), hex_string.data() + hex_string.size(), color, 16);
    if (ec != std::errc() || ptr != hex_string.data() + hex_string.size()) {
      return ColorCode::Black;
    }
    return DecodeColor(color);
  }

  return ColorCode::Black;
}

std::string ColorToString(Color color) {
  return std::format("#{:08X}", EncodeColor(color));
}

std::ostream& operator<<(std::ostream& stream, Color color) {
  if (int index = FindColor(color); index != -1)
    return stream << GetColorDebugName(index);

  const aui::Rgba& rgba = color.rgba();
  return stream << "(R: " << static_cast<int>(rgba.r)
                << ", G: " << static_cast<int>(rgba.g)
                << ", B: " << static_cast<int>(rgba.b)
                << ", A: " << static_cast<int>(rgba.a) << ")";
}

}  // namespace aui
