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

  // Opens the preferences dialog. Reached from Settings > Settings... and from
  // the activity rail's pinned Settings utility, which share this one command
  // so the two entry points cannot diverge. Defaulted rather than pure: the
  // preferences surface is a desktop dialog, and the Wt shell has none — it
  // keeps offering the toggles as menu items, so it wants no-op here rather
  // than an override that does nothing.
  virtual void ShowSettingsDialog() {}

 protected:
  ~MainWindowInterface() = default;
};
