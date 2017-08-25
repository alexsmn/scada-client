#pragma once

#include <memory>

namespace gfx {
class Point;
}

class OpenedView;
class WindowDefinition;

class ViewManagerDelegate {
 public:
  virtual std::unique_ptr<OpenedView> OnCreateView(WindowDefinition& definition) = 0;
  virtual void OnViewClosed(OpenedView& view, WindowDefinition& definition) = 0;
  virtual void OnActiveViewChanged(OpenedView* view) = 0;
  virtual void OnShowTabPopupMenu(OpenedView& view, const gfx::Point& point) = 0;
};
