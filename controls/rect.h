#pragma once

namespace aui {

struct Rect {
  int x = 0;
  int y = 0;
  int width = 0;
  int height = 0;

  constexpr bool empty() const noexcept { return width == 0 || height == 0; }
};

}  // namespace aui
