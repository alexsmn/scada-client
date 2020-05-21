#pragma once

#include <SkColor.h>

namespace aui {

using NativeColor = SkColor;

struct Color {
  Color(const Rgba& rgba) noexcept
      : native_color_{SkColorSetARGB(rgba.r, rgba.g, rgba.b, rgba.a)} {}

  static constexpr Color FromSkColor(::SkColor sk_color) noexcept {
    return FromNativeColor(sk_color);
  }

  static constexpr Color FromNativeColor(NativeColor native_color) noexcept {
    return Color{native_color};
  }

  constexpr NativeColor native_color() const noexcept { return native_color_; }

  constexpr SkColor sk_color() const noexcept { return native_color_; }

  bool operator==(const Color& other) const noexcept {
    return native_color_ == other.native_color_;
  }
  bool operator!=(const Color& other) const noexcept {
    return !operator==(other);
  }
  bool operator<(const Color& other) const noexcept {
    return native_color_ < other.native_color_;
  }

 private:
  explicit constexpr Color(NativeColor native_color) noexcept
      : native_color_{native_color} {}

  NativeColor native_color_;
};

}  // namespace aui
