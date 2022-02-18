#pragma once

#include "components/main/opened_view.h"
#include "controls/point.h"

#include <memory>

class WindowDefinition;

class ViewManagerDelegate {
 public:
  virtual std::unique_ptr<OpenedView> OnCreateView(
      WindowDefinition& definition) = 0;

  virtual void OnViewClosed(OpenedView& view) = 0;

  virtual void OnActiveViewChanged(OpenedView* view) = 0;

  virtual void OnShowTabPopupMenu(OpenedView& view, const aui::Point& point) = 0;
};
