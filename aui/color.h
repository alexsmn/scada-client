#pragma once

#include <ostream>
#include <string>
#include <string_view>

namespace aui {

struct Rgba {
  std::uint8_t r = 0;
  std::uint8_t g = 0;
  std::uint8_t b = 0;
  std::uint8_t a = 255;

  constexpr auto operator<=>(const Rgba& other) const noexcept = default;
};

}  // namespace aui

#if defined(UI_QT)
#include "aui/qt/color_qt.h"
#elif defined(UI_WT)
#include "aui/wt/color_wt.h"
#endif

#if defined(_WIN32)
#include "aui/color_win.h"
#endif

namespace aui {

size_t GetColorCount();

Color GetColor(int index);
int FindColor(Color color);

std::u16string_view GetColorName(int index);
int FindColorName(std::u16string_view name);

std::string_view GetColorDebugName(int index);

Color StringToColor(std::string_view str);
std::string ColorToString(Color color);

struct ColorCode {
  static inline constexpr Rgba Transparent{0, 0, 0, 0};
  static inline constexpr Rgba Black{0, 0, 0};
  static inline constexpr Rgba White{255, 255, 255};
  static inline constexpr Rgba Gray{136, 136, 126};
  static inline constexpr Rgba DarkGray{227, 227, 227};
  static inline constexpr Rgba Red{255, 0, 0};
  static inline constexpr Rgba Green{0, 255, 0};
  static inline constexpr Rgba Blue{0, 0, 255};
  static inline constexpr Rgba Yellow{255, 255, 0};
  static inline constexpr Rgba Cyan{0, 100, 100};
  static inline constexpr Rgba Crimson{220, 20, 60};
};

std::ostream& operator<<(std::ostream& stream, Color color);

}  // namespace aui
