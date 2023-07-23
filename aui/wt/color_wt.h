#pragma once

#include "aui/color.h"

#include <Wt/WColor.h>

namespace aui {

class Color {
 public:
  Color(Wt::WColor wcolor) : wcolor_{wcolor} {}
  Color(const Rgba& rgba) noexcept : wcolor_{rgba.r, rgba.g, rgba.b, rgba.a} {}

  Wt::WColor wcolor() const noexcept { return wcolor_; }
  Wt::WColor native_color() const noexcept { return wcolor_; }

  Rgba rgba() const noexcept {
    return {.r = static_cast<std::uint8_t>(wcolor_.red()),
            .g = static_cast<std::uint8_t>(wcolor_.green()),
            .b = static_cast<std::uint8_t>(wcolor_.blue()),
            .a = static_cast<std::uint8_t>(wcolor_.alpha())};
  }

  bool operator==(const Color& other) const noexcept = default;

  auto operator<=>(const Color& other) const noexcept {
    return rgba() <=> other.rgba();
  }

 private:
  Wt::WColor wcolor_;
};

}  // namespace aui
