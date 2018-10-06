#pragma once

#include "base/strings/string16.h"
#include "controls/types.h"

#include <SkColor.h>

namespace palette {

size_t GetColorCount();

SkColor GetColor(int index);
int FindColor(SkColor color);

const base::char16* GetColorName(int index);
int FindColorName(const base::char16* str);

SkColor StringToColor(base::StringPiece str);
std::string ColorToString(SkColor color);

}  // namespace palette
