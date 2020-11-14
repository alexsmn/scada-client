#include "controls/color.h"

#include "base/containers/span.h"
#include "base/strings/stringprintf.h"

#include <cassert>
#include <string.h>

namespace aui {

struct ColorEntry {
  std::wstring_view name;
  Rgba color;
};

constexpr ColorEntry kColorEntries[] = {
    {L"Прозрачный", ColorCode::Transparent},
    {L"Черный", ColorCode::Black},
    {L"Белый", ColorCode::White},
    {L"Красный", ColorCode::Red},
    {L"Зеленый", ColorCode::Green},
    {L"Синий", ColorCode::Blue},
    {L"Желтый", ColorCode::Yellow},
    {L"Голубой", ColorCode::Cyan},
    {L"Малиновый", ColorCode::Crimson},
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

std::wstring_view GetColorName(int index) {
  const ColorEntry* entry = GetColorEntry(index);
  return entry ? entry->name : std::wstring_view{};
}

int FindColorName(std::wstring_view str) {
  for (size_t i = 0; i < std::size(kColorEntries); i++) {
    if (_wcsicmp(std::wstring{str}.c_str(),
                 std::wstring{kColorEntries[i].name}.c_str()) == 0) {
      return i;
    }
  }
  return -1;
}

Color StringToColor(std::string_view str) {
  if (!str.empty() && str[0] == '#') {
    unsigned color = 0;
    if (sscanf(std::string{str}.c_str(), "#%08X", &color) != 1)
      return ColorCode::Black;
    return aui::Color::FromSkColor(color);
  }

  return ColorCode::Black;
}

std::string ColorToString(Color color) {
  return base::StringPrintf("#%08X", static_cast<unsigned>(color.sk_color()));
}

}  // namespace aui
