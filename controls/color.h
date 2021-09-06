#pragma once

#include "controls/types.h"

#include <string>

namespace aui {

struct Rgba {
  int r = 0;
  int g = 0;
  int b = 0;
  int a = 255;

  constexpr bool operator==(const Rgba& other) const noexcept {
    return r == other.r && g == other.g && b == other.b && a == other.a;
  }
};

}  // namespace aui

#if defined(UI_QT)
#include "controls/qt/color_qt.h"
#elif defined(UI_VIEWS)
#include "controls/views/color_views.h"
#elif defined(UI_WT)
#include "controls/wt/color_wt.h"
#endif

#include <SkColor.h>

// For convenience.
inline SkColor ToSkColor(SkColor color) {
  return color;
}

namespace aui {

size_t GetColorCount();

Color GetColor(int index);
int FindColor(Color color);

std::wstring_view GetColorName(int index);
int FindColorName(std::wstring_view);

Color StringToColor(std::string_view str);
std::string ColorToString(Color color);

struct ColorCode {
  static inline constexpr Rgba Transparent{0, 0, 0, 0};
  static inline constexpr Rgba Black{0, 0, 0};
  static inline constexpr Rgba White{255, 255, 255};
  static inline constexpr Rgba Red{255, 0, 0};
  static inline constexpr Rgba Green{0, 255, 0};
  static inline constexpr Rgba Blue{0, 0, 255};
  static inline constexpr Rgba Yellow{0, 0, 255};
  static inline constexpr Rgba Cyan{0, 0, 255};
  static inline constexpr Rgba Crimson{0, 0, 255};
};

}  // namespace aui
