#pragma once

namespace aui {

struct Size {
  int width = 0;
  int height = 0;

  constexpr bool empty() const noexcept { return width == 0 || height == 0; }

  constexpr bool operator==(const Size& other) const noexcept {
    return width == other.width && height == other.height;
  }
};

}  // namespace aui
