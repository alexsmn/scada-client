#pragma once

#include "aui/severity_colors.h"

#include <QColor>
#include <QPixmap>

class QFont;

namespace scada::aui {

// The diameter, in logical pixels, of a tree row's status glyph for rows set
// in `font`: about two thirds of the font's line height, so the glyph grows
// with the user's font size and the bad band's «?» stays legible, but never
// smaller than 9 px nor taller than the style's small icon.
int StatusGlyphDiameter(const QFont& font);

// Draws a data-quality band as a shape as well as a colour, so the band never
// rests on colour alone (docs/client/ux/principles.md §5):
//  - good: a filled disc;
//  - uncertain: a ring whose left half is filled;
//  - bad: a ring with a «?» inside (principles §5's "bad-quality value also
//    carries a `?`").
// The pixmap is `diameter` logical px square at `device_pixel_ratio`, with a
// transparent background.
QPixmap DrawStatusGlyph(Quality quality,
                        const QColor& color,
                        int diameter,
                        qreal device_pixel_ratio);

}  // namespace scada::aui
