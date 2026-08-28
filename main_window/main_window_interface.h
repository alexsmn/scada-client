#pragma once

#include "base/awaitable.h"

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

  virtual Awaitable<OpenedViewInterface*> OpenView(
      const WindowDefinition& window_definition,
      bool activate = true) = 0;

  [[nodiscard]] virtual OpenedViewInterface* FindViewByType(
      std::string_view window_type) const = 0;

  // Layout.

  virtual void SplitView(OpenedViewInterface& view, bool vertically) = 0;

  // Preferences.

  // Opens the Settings surface over this window. Reached from Settings >
  // Settings... and from the activity rail's pinned Settings utility, which
  // share this one command so the two entry points cannot diverge.
  //
  // A surface rather than a dialog, and an overlay rather than a workspace tab:
  // it covers the whole workbench below the status strip — rail, Explorer,
  // tab strip and the bottom events dock — and closing it puts nothing back,
  // because it moved nothing. See `settings/qt/settings_panel.h`.
  //
  // Defaulted rather than pure so an implementation with no preferences
  // surface — a test double, or a build without the Qt UI config — need not
  // override it.
  virtual void ShowSettings() {}

 protected:
  ~MainWindowInterface() = default;
};
