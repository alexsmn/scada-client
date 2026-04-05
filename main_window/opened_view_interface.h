#pragma once

#include "base/promise.h"
#include "profile/window_definition.h"

class ContentsModel;
struct WindowInfo;

class OpenedViewInterface {
 public:
  [[nodiscard]] virtual const WindowInfo& GetWindowInfo() const = 0;

  [[nodiscard]] virtual std::u16string GetWindowTitle() const = 0;

  virtual void SetWindowTitle(std::u16string_view title) = 0;

  // Updates modified flag, and so is not const.
  [[nodiscard]] virtual WindowDefinition Save() = 0;

  [[nodiscard]] virtual ContentsModel* GetContents() = 0;

  virtual void Select(const scada::NodeId& node_id) = 0;

  [[nodiscard]] virtual promise<WindowDefinition> GetOpenWindowDefinition(
      const WindowInfo* window_info) const = 0;

 protected:
  ~OpenedViewInterface() = default;
};