#pragma once

#include "base/strings/string16.h"
#include "controls/types.h"
#include "core/SkColor.h"

#if defined(UI_QT)
#include <qcolor.h>
#endif

namespace palette {

size_t GetColorCount();

SkColor GetColor(int index);
int FindColor(SkColor color);

const base::char16* GetColorName(int index);
int FindColorName(const base::char16* str);

SkColor StringToColor(const std::string& str);
std::string ColorToString(SkColor color);

} // namespace palette

#if defined(UI_QT)
QColor ColorToQt(SkColor color);
SkColor ColorFromQt(QColor qcolor);
#endif