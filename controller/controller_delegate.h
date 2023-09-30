#pragma once

#include "aui/point.h"

#include <string_view>

namespace ui {
class MenuModel;
}

class ContentsModel;
class ContentsObserver;
class NodeRef;
class WindowDefinition;

class ControllerDelegate {
 public:
  virtual void SetTitle(std::u16string_view title) = 0;

  // |point| is in _screen_ coordinates.
  // |right_click| should be set if popup is initated by right-click.
  virtual void ShowPopupMenu(unsigned resource_id,
                             const aui::Point& point,
                             bool right_click) = 0;

  virtual void SetModified(bool modified) = 0;

  virtual void Close() = 0;

  virtual void OpenView(const WindowDefinition& def) = 0;

  virtual void ExecuteDefaultNodeCommand(const NodeRef& node) = 0;

  virtual ContentsModel* GetActiveContentsModel() = 0;
  virtual void AddContentsObserver(ContentsObserver& observer) = 0;
  virtual void RemoveContentsObserver(ContentsObserver& observer) = 0;

  virtual void Focus() = 0;
};
