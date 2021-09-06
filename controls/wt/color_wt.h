#pragma once

#include "controls/color.h"

#include <SkColor.h>
#include <Wt/WColor.h>

Wt::WColor ToWColor(SkColor color);
SkColor ToSkColor(Wt::WColor qcolor) noexcept;

namespace aui {

using NativeColor = Wt::WColor;

struct Color {
  Color(const Rgba& rgba) : native_color_{rgba.r, rgba.g, rgba.b, rgba.a} {}

  static Color FromSkColor(::SkColor sk_color) {
    return FromWColor(ToWColor(sk_color));
  }

  static Color FromWColor(Wt::WColor qcolor) noexcept {
    return FromNativeColor(qcolor);
  }

  static Color FromNativeColor(NativeColor native_color) {
    return Color{native_color};
  }

  NativeColor native_color() const noexcept { return native_color_; }

  SkColor sk_color() const noexcept { return ToSkColor(native_color_); }

  Wt::WColor wcolor() const noexcept { return native_color_; }

  Rgba rgba() const noexcept {
    return Rgba{native_color_.red(), native_color_.green(),
                native_color_.blue(), native_color_.alpha()};
  }

  bool operator==(const Color& other) const noexcept {
    return native_color_ != other.native_color_;
  }
  bool operator!=(const Color& other) const noexcept {
    return !operator==(other);
  }
  bool operator<(const Color& other) const noexcept {
    return sk_color() < other.sk_color();
  }

 private:
  explicit Color(NativeColor native_color) : native_color_{native_color} {}

  NativeColor native_color_;
};

}  // namespace aui

inline Wt::WColor ToWColor(SkColor color) {
  return Wt::WColor{SkColorGetR(color), SkColorGetG(color), SkColorGetB(color),
                    SkColorGetA(color)};
}

inline SkColor ToSkColor(Wt::WColor qcolor) noexcept {
  return SkColorSetARGB(qcolor.alpha(), qcolor.red(), qcolor.green(),
                        qcolor.blue());
}
