#pragma once

#include "base/lifetime.h"
#include "profile/page_layout.h"
#include "profile/window_definition.h"
#include <boost/json.hpp>

class Page {
 public:
  Page() = default;

  Page(const Page& source);
  Page& operator=(const Page& source);

  std::u16string GetTitle() const;

  WindowDefinition& AddWindow(const WindowDefinition& window);
  WindowDefinition* FindWindowDef(int id);
  const WindowDefinition* FindWindowDef(int id) const;
  int FindWindowDef(const WindowDefinition& window) const;
  void DeleteWindow(int index);
  void Clear();

  int GetWindowCount() const { return windows_.size(); }
  WindowDefinition& GetWindow(int index) const SCADA_LIFETIME_BOUND {
    return *windows_[index];
  }

  void Load(const boost::json::value& value);
  boost::json::value Save(bool current) const;

  int id = 0;
  std::u16string title;
  // The page's position in the activity rail and the Page menu. Explicit
  // because `Profile::pages` is keyed by id, which cannot express an order the
  // operator can rearrange. 0 means "never ordered" — those sort after the
  // ordered ones, by id, so a profile written before reordering existed keeps
  // its historical order.
  int order = 0;
  // The operator's chosen rail icon, as a `PageIcon::key`
  // (`main_window/page_icons.h`). Empty means none was chosen, and the rail
  // falls back to drawing the page's position — which is also what happens for
  // a key this build does not know, so a profile written by a newer build
  // still opens.
  std::string icon;

  PageLayout layout;

 private:
  int NewWindowId();

  using Windows = std::vector<std::unique_ptr<WindowDefinition>>;
  Windows windows_;
};
