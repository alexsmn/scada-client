#pragma once

#include "aui/color.h"

#include <Windows.h>

namespace aui {

inline constexpr Rgba COLORREFToRgba(COLORREF colorref) noexcept {
  return Rgba{
      GetRValue(colorref),
      GetGValue(colorref),
      GetBValue(colorref),
  };
}

inline Color COLORREFToColor(COLORREF colorref) noexcept {
  return COLORREFToRgba(colorref);
}

inline COLORREF ToCOLORREF(Color color) {
  const Rgba& rgba = color.rgba();
  // COLORREF doesn't support an alpha channel.
  assert(rgba.a == 0);
  return RGB(rgba.r, rgba.g, rgba.b);
}

}  // namespace aui
