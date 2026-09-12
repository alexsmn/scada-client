#pragma once

#include "main_window/opened_view/opened_view.h"
#include "aui/point.h"

#include <memory>

class WindowDefinition;

class ViewManagerDelegate {
 public:
  virtual std::unique_ptr<OpenedView> OnCreateView(
      WindowDefinition& definition) = 0;

  virtual void OnViewClosed(OpenedView& view) = 0;

  virtual void OnActiveViewChanged(OpenedView* view) = 0;

  virtual void OnShowTabPopupMenu(OpenedView& view,
                                  const scada::aui::Point& point) = 0;

  // The tab strip's "new view" button was pressed: offer the views the current
  // selection can be opened in, plus the ones that open empty
  // (docs/product/ui-mockups/authoring.md 4b). The strip the button belongs to
  // is already the active one, so opening a view beside the active one puts the
  // new tab where the user clicked.
  virtual void OnShowNewViewMenu(const scada::aui::Point& point) = 0;
};
