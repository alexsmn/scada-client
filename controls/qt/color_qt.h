#pragma once

#include "controls/color.h"

#include <QColor>
#include <SkColor.h>

QColor ToQColor(SkColor color);
SkColor ToSkColor(QColor qcolor) noexcept;

namespace aui {

using NativeColor = ::QColor;

struct Color {
  Color(const Rgba& rgba) : native_color_{rgba.r, rgba.g, rgba.b, rgba.a} {}

  static Color FromSkColor(::SkColor sk_color) {
    return FromQColor(ToQColor(sk_color));
  }

  static Color FromQColor(::QColor qcolor) noexcept {
    return FromNativeColor(qcolor);
  }

  static Color FromNativeColor(NativeColor native_color) {
    return Color{native_color};
  }

  NativeColor native_color() const noexcept { return native_color_; }

  SkColor sk_color() const noexcept { return ToSkColor(native_color_); }

  QColor qcolor() const noexcept { return native_color_; }

  bool operator==(const Color& other) const noexcept = default;
  bool operator!=(const Color& other) const noexcept = default;
  bool operator<(const Color& other) const noexcept {
    return native_color_.rgba() < other.native_color_.rgba();
  }

 private:
  explicit Color(NativeColor native_color) : native_color_{native_color} {}

  NativeColor native_color_;
};

}  // namespace aui

inline QColor ToQColor(SkColor color) {
  return QColor{SkColorGetR(color), SkColorGetG(color), SkColorGetB(color),
                SkColorGetA(color)};
}

inline SkColor ToSkColor(QColor qcolor) noexcept {
  return SkColorSetARGB(qcolor.alpha(), qcolor.red(), qcolor.green(),
                        qcolor.blue());
}
