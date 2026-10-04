#pragma once

#include "main_window/pages/page_switcher.h"

#include <functional>
#include <memory>
#include <string>
#include <string_view>

class ActivityBar;
class QMenu;
class QPoint;
class QWidget;

// What RailPagesController needs from the window it serves.
struct RailPagesControllerContext {
  // The rail whose pages group this controller drives. Must outlive it.
  ActivityBar& activity_bar_;
  // Builds the page list and switching policy, shared with the Page menu.
  PageSwitcherContext page_switcher_;
  // Runs a registered command through the shell's command resolution, doing
  // nothing when it does not resolve or is disabled. New, Rename, Duplicate
  // and Delete all go this way, so the rail cannot drift from what the Page
  // menu and the Ctrl-K palette do.
  std::function<void(unsigned command_id)> execute_command_;
  // Parent for the per-page context menu.
  QWidget& menu_parent_;
};

// The activity rail's pages group: one button per profile page, the "+" that
// creates one, drag-to-reorder, and the per-page context menu (Open, Rename,
// Duplicate, Move up/down, Icon, Delete, New page).
//
// Split out of MainWindow on 2026-10-04 (backlog 722). The rail widget stays
// MainWindow's; this owns what its pages group shows and what clicking it
// does. Re-derive with Refresh() whenever the page list or the open page may
// have changed — a page open, a rename, a reorder.
class RailPagesController {
 public:
  explicit RailPagesController(RailPagesControllerContext&& context);
  ~RailPagesController();

  RailPagesController(const RailPagesController&) = delete;
  RailPagesController& operator=(const RailPagesController&) = delete;

  // Rebuilds the rail's page buttons from the profile and re-marks the open
  // page.
  void Refresh();

  // The context menu for `page_id`, as the rail's right-click shows it, built
  // but not run. Separate from ShowPageContextMenu so a test or a capture can
  // read it: QMenu::exec runs a nested event loop that offscreen nothing
  // dismisses.
  std::unique_ptr<QMenu> BuildPageContextMenu(int page_id);

  // Builds the context menu for `page_id` and runs it at `global_pos`.
  void ShowPageContextMenu(int page_id, const QPoint& global_pos);

  // The rail icon currently set on `page_id`, or empty when it has none. Read
  // back from the profile rather than cached, so the context menu's check
  // mark cannot disagree with what the rail draws.
  std::string PageIconFor(int page_id) const;

  // Sets `page_id`'s rail icon and redraws the rail. Empty `key` clears it.
  void SetPageIcon(int page_id, std::string_view key);

 private:
  // Moves `page_id` to `new_index` and redraws the rail. The Page menu reads
  // the same ordered list, so it follows without further wiring.
  void ReorderPage(int page_id, int new_index);

  ActivityBar& activity_bar_;
  const std::function<void(unsigned command_id)> execute_command_;
  QWidget& menu_parent_;
  PageSwitcher page_switcher_;
};
