#pragma once

#include "controls/color.h"

#include <QColor>
#include <QRgb>

namespace aui {

class Color {
 public:
  constexpr Color(QColor qcolor) : qcolor_{qcolor} {}
  Color(const Rgba& rgba) noexcept : qcolor_{rgba.r, rgba.g, rgba.b, rgba.a} {}

  QColor qcolor() const noexcept { return qcolor_; }
  QColor native_color() const noexcept { return qcolor_; }

  Rgba rgba() const noexcept {
    const QRgb& qrgba = qcolor_.rgba();
    return {.r = static_cast<std::uint8_t>(qRed(qrgba)),
            .g = static_cast<std::uint8_t>(qGreen(qrgba)),
            .b = static_cast<std::uint8_t>(qBlue(qrgba)),
            .a = static_cast<std::uint8_t>(qAlpha(qrgba))};
  }

  bool operator==(const Color& other) const noexcept = default;

  auto operator<=>(const Color& other) const noexcept {
    return rgba() <=> other.rgba();
  }

 private:
  QColor qcolor_;
};

}  // namespace aui
