#pragma once

#include "base/promise.h"

class OpenedViewInterface;
class Page;
class WindowDefinition;

class MainWindowInterface {
 public:
  [[nodiscard]] virtual int GetMainWindowId() const = 0;

  // Pages.

  [[nodiscard]] virtual const Page& GetCurrentPage() const = 0;
  virtual void OpenPage(const Page& page) = 0;
  virtual void SetCurrentPageTitle(std::u16string_view title) = 0;
  virtual void SaveCurrentPage() = 0;
  virtual void DeleteCurrentPage() = 0;

  // Views.

  [[nodiscard]] virtual OpenedViewInterface* GetActiveView() const = 0;
  [[nodiscard]] virtual OpenedViewInterface* GetActiveDataView() const = 0;
  virtual void ActivateView(const OpenedViewInterface& view) = 0;

  [[nodiscard]] virtual std::vector<OpenedViewInterface*> GetOpenedViews()
      const = 0;

  virtual promise<OpenedViewInterface*> OpenView(
      const WindowDefinition& window_definition,
      bool activate = true) = 0;

  [[nodiscard]] virtual OpenedViewInterface* FindViewByType(
      std::string_view window_type) const = 0;

  // Layout.

  virtual void SplitView(OpenedViewInterface& view, bool vertically) = 0;

 protected:
  ~MainWindowInterface() = default;
};
