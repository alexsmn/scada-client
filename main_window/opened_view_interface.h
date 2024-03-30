#pragma once

#include "base/promise.h"

class WindowDefinition;
struct WindowInfo;

class OpenedViewInterface {
 public:
  virtual ~OpenedViewInterface() = default;

  virtual promise<WindowDefinition> GetOpenWindowDefinition(
      const WindowInfo* window_info) const = 0;
};