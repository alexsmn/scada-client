#pragma once

#include "controls/color.h"

namespace gfx {
class Canvas;
class Font;
}

void MeasureColoredString(gfx::Canvas* canvas, const gfx::Font& font,
                          SkColor color, RECT& rect,
                          const std::wstring& text, int format);
void DrawColoredString(gfx::Canvas* canvas, const gfx::Font& font,
                       SkColor color, const RECT& rect,
                       const std::wstring& text, int format);
