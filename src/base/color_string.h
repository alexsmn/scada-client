#pragma once

void MeasureColoredString(gfx::Canvas* canvas, const gfx::Font& font,
                          SkColor color, RECT& rect,
                          const base::string16& text, int format);
void DrawColoredString(gfx::Canvas* canvas, const gfx::Font& font,
                       SkColor color, const RECT& rect,
                       const base::string16& text, int format);
