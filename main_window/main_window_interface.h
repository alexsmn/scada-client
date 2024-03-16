#pragma once

#include "scada/status_promise.h"

class OpenedView;
class Page;
class WindowDefinition;

class MainWindowInterface {
 public:
  virtual ~MainWindowInterface() = default;

  virtual int GetMainWindowId() const = 0;

  // Pages.

  virtual const Page& GetCurrentPage() const = 0;
  virtual void OpenPage(const Page& page) = 0;
  virtual void SetCurrentPageTitle(std::u16string_view title) = 0;
  virtual void SaveCurrentPage() = 0;
  virtual void DeleteCurrentPage() = 0;

  virtual OpenedView* GetActiveView() = 0;
  virtual OpenedView* GetActiveDataView() = 0;

  // Views.

  virtual scada::status_promise<OpenedView*> OpenView(
      const WindowDefinition& window_definition,
      bool make_active = true) = 0;
};
