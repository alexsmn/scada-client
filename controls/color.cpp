#include "controls/color.h"

#include "base/containers/span.h"
#include "base/strings/stringprintf.h"

#include <cassert>
#include <string.h>

namespace aui {

struct ColorEntry {
  base::StringPiece16 name;
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

base::StringPiece16 GetColorName(int index) {
  const ColorEntry* entry = GetColorEntry(index);
  return entry ? entry->name : base::StringPiece16{};
}

int FindColorName(base::StringPiece16 str) {
  for (size_t i = 0; i < std::size(kColorEntries); i++) {
    if (_wcsicmp(str.as_string().c_str(),
                 kColorEntries[i].name.as_string().c_str()) == 0) {
      return i;
    }
  }
  return -1;
}

Color StringToColor(base::StringPiece str) {
  if (!str.empty() && str[0] == '#') {
    unsigned color = 0;
    if (sscanf(str.as_string().c_str(), "#%08X", &color) != 1)
      return ColorCode::Black;
    return aui::Color::FromSkColor(color);
  }

  return ColorCode::Black;
}

std::string ColorToString(Color color) {
  return base::StringPrintf("#%08X", static_cast<unsigned>(color.sk_color()));
}

}  // namespace aui
