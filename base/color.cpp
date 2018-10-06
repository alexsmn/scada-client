#include "base/color.h"
#include "base/strings/stringprintf.h"

#include <string.h>
#include <cassert>

namespace palette {

struct ColorEntry {
  const base::char16* name;
  SkColor color;
};

static const ColorEntry* GetColorEntries(size_t* count) {
  static const ColorEntry kColorEntries[] = {
      {L"Прозрачный", SkColorSetARGB(0, 0, 0, 0)},
      {L"Черный", SK_ColorBLACK},
      {L"Белый", SK_ColorWHITE},
      {L"Синий", SK_ColorBLUE},
      {L"Красный", SK_ColorRED},
      {L"Зеленый", SK_ColorGREEN},
      {L"Желтый", SK_ColorYELLOW},
      {L"Голубой", SkColorSetRGB(0, 255, 255)},
      {L"Малиновый", SkColorSetRGB(255, 0, 255)},
  };

  if (count)
    *count = _countof(kColorEntries);

  return kColorEntries;
}

static const ColorEntry* GetColorEntry(int index) {
  size_t count = 0;
  const ColorEntry* entries = GetColorEntries(&count);
  if (index >= 0 && index < static_cast<int>(count))
    return &entries[index];
  return NULL;
}

size_t GetColorCount() {
  size_t count = 0;
  GetColorEntries(&count);
  return count;
}

SkColor GetColor(int index) {
  const ColorEntry* entry = GetColorEntry(index);
  return entry ? entry->color : SK_ColorBLACK;
}

int FindColor(SkColor color) {
  size_t count = 0;
  const ColorEntry* entries = GetColorEntries(&count);
  for (size_t i = 0; i < count; i++)
    if (entries[i].color == color)
      return i;
  return -1;
}

const base::char16* GetColorName(int index) {
  const ColorEntry* entry = GetColorEntry(index);
  return entry ? entry->name : L"";
}

int FindColorName(const base::char16* str) {
  size_t count = 0;
  const ColorEntry* entries = GetColorEntries(&count);
  for (size_t i = 0; i < count; i++)
    if (_wcsicmp(str, entries[i].name) == 0)
      return i;
  return -1;
}

SkColor StringToColor(base::StringPiece str) {
  if (!str.empty() && str[0] == '#') {
    unsigned color = 0;
    if (sscanf(str.as_string().c_str(), "#%08X", &color) != 1)
      return SK_ColorBLACK;
    return static_cast<SkColor>(color);
  }

  return SK_ColorBLACK;
}

std::string ColorToString(SkColor color) {
  return base::StringPrintf("#%08X", color);
}

}  // namespace palette
