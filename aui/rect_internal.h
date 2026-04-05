#pragma once

namespace aui::internal {

// Micks Qt's `QRect`.
class Rect {
 public:
  // Constructs a null rectangle.
  constexpr Rect() = default;

  constexpr Rect(int x, int y, int width, int height)
      : x_(x), y_(y), width_(width), height_(height) {}

  [[nodiscard]] constexpr int x() const { return x_; }
  [[nodiscard]] constexpr int y() const { return y_; }
  [[nodiscard]] constexpr int width() const { return width_; }
  [[nodiscard]] constexpr int height() const { return height_; }

  [[nodiscard]] constexpr bool isNull() const noexcept {
    return width_ == 0 || height_ == 0;
  }

 private:
  int x_ = 0;
  int y_ = 0;
  int width_ = 0;
  int height_ = 0;
};

}  // namespace aui::internal