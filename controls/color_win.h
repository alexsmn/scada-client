#pragma once

#include "controls/color.h"

namespace aui {

inline constexpr Rgba ToRgba(COLORREF colorref) noexcept {
  return Rgba{
      GetRValue(colorref),
      GetGValue(colorref),
      GetBValue(colorref),
  };
}

inline Color ToColor(COLORREF colorref) noexcept {
  return ToRgba(colorref);
}

}  // namespace aui
