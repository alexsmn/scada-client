#pragma once

#include "base/any_executor.h"
#include "base/cancelation.h"

#include <string>
#include <vector>

class DialogService;
class MainWindowInterface;
class MainWindowManager;
class Page;
class Profile;

// One entry of the profile's page list, flattened for display.
struct PageEntry {
  int page_id = 0;
  // The page's display name. Uses Page::GetTitle(), which synthesizes a name
  // from the page's windows when no explicit title was set.
  std::u16string title;
  // This page is the one the window currently shows.
  bool current = false;
  // Another main window already has this page open, so activating it would be
  // refused. Surfaced so a list can dim the row instead of letting the operator
  // discover the refusal through a message box.
  bool opened_elsewhere = false;
  // The page's sort key (Page::order). 0 means the page predates reordering.
  int order = 0;
  // The page's chosen rail icon (`Page::icon`), a `PageIcon::key`. Empty when
  // the operator has not picked one.
  std::string icon;
};

struct PageSwitcherContext {
  const AnyExecutor executor_;
  Profile& profile_;
  MainWindowInterface& main_window_;
  MainWindowManager& main_window_manager_;
  DialogService& dialog_service_;
};

// The profile's page list and the policy for switching between pages.
//
// Extracted from PageMenuModel so the main menu and the activity rail's Pages
// pane present the same list and obey the same rules — the revert confirmation,
// the refusal to open a page twice, and saving the outgoing page.
class PageSwitcher : private PageSwitcherContext {
 public:
  explicit PageSwitcher(PageSwitcherContext&& context);
  ~PageSwitcher();

  // The profile's pages in the operator's order (Page::order, then id for
  // pages that predate reordering). Every page surface reads this, so the rail
  // and the Page menu always agree.
  std::vector<PageEntry> ListPages() const;

  // Moves `page_id` to `new_index` in the list above and renumbers the rest.
  // A no-op when the id is unknown or already at that index.
  void ReorderPage(int page_id, int new_index);

  // Sets `page_id`'s rail icon to a `PageIcon::key`, or clears it when `key` is
  // empty. A no-op when the id is unknown. Rejects a key this build cannot
  // draw, so a typo cannot persist a page that renders as its ordinal for ever
  // with no way to tell why.
  void SetPageIcon(int page_id, std::string_view key);

  // Switches to `page_id`. Activating the page that is already open is a
  // revert: it asks for confirmation and then re-opens the persisted copy,
  // discarding unsaved layout changes. Switching to another page saves the
  // current one first, and is refused when that page is open in another window.
  void ActivatePage(int page_id);

 private:
  void OpenPageHelper(const Page& page, bool revert);

  Cancelation cancelation_;
};
