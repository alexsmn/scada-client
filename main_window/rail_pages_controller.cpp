#include "main_window/rail_pages_controller.h"

#include "aui/translation.h"
#include "main_window/activity_bar_qt.h"
#include "main_window/page_icons.h"
#include "resources/common_resources.h"

#include <QAction>
#include <QMenu>
#include <QObject>
#include <QPoint>

#include <algorithm>
#include <utility>
#include <vector>

RailPagesController::RailPagesController(RailPagesControllerContext&& context)
    : activity_bar_{context.activity_bar_},
      execute_command_{std::move(context.execute_command_)},
      menu_parent_{context.menu_parent_},
      page_switcher_{std::move(context.page_switcher_)} {
  activity_bar_.SetPageCallbacks(
      [this](int page_id) { page_switcher_.ActivatePage(page_id); },
      [this] { execute_command_(ID_PAGE_NEW); },
      [this](int page_id, const QPoint& global_pos) {
        ShowPageContextMenu(page_id, global_pos);
      },
      [this](int page_id, int new_index) { ReorderPage(page_id, new_index); });

  Refresh();
}

RailPagesController::~RailPagesController() = default;

void RailPagesController::Refresh() {
  std::vector<ActivityBar::PageButton> buttons;
  int active_page_id = 0;
  for (const PageEntry& entry : page_switcher_.ListPages()) {
    buttons.push_back(
        ActivityBar::PageButton{.page_id = entry.page_id,
                                .title = entry.title,
                                .icon_key = entry.icon,
                                .opened_elsewhere = entry.opened_elsewhere});
    if (entry.current)
      active_page_id = entry.page_id;
  }

  activity_bar_.SetPages(std::move(buttons));
  activity_bar_.SetActivePage(active_page_id);
}

std::string RailPagesController::PageIconFor(int page_id) const {
  for (const PageEntry& entry : page_switcher_.ListPages()) {
    if (entry.page_id == page_id)
      return entry.icon;
  }
  return {};
}

void RailPagesController::SetPageIcon(int page_id, std::string_view key) {
  page_switcher_.SetPageIcon(page_id, key);
  // The rail reads the icon from the profile, so redraw the buttons rather
  // than mutating the one that was clicked — same path a rename takes.
  Refresh();
}

void RailPagesController::ReorderPage(int page_id, int new_index) {
  page_switcher_.ReorderPage(page_id, new_index);
  Refresh();
}

std::unique_ptr<QMenu> RailPagesController::BuildPageContextMenu(int page_id) {
  auto menu = std::make_unique<QMenu>(&menu_parent_);

  const std::vector<PageEntry> pages = page_switcher_.ListPages();
  const auto entry = std::ranges::find(pages, page_id, &PageEntry::page_id);

  // Rename and Delete act on the *current* page (that is what ID_PAGE_RENAME
  // and ID_PAGE_DELETE mean), so they are enabled only on the page that is
  // open. Anything else would silently rename the wrong one.
  const bool is_current = entry != pages.end() && entry->current;

  auto add = [&](unsigned command_id, const char* label, bool enabled) {
    QAction* action =
        menu->addAction(QString::fromStdU16String(Translate(label)));
    action->setEnabled(enabled);
    QObject::connect(action, &QAction::triggered, menu.get(),
                     [this, command_id] { execute_command_(command_id); });
  };

  // Opening the right-clicked page is the menu's primary action, so it leads
  // and is bold. Absent when that page is already open — "Open page" on the
  // page you are looking at would mean the revert, which is not what the
  // reader would expect from it.
  if (!is_current && entry != pages.end() && !entry->opened_elsewhere) {
    QAction* open =
        menu->addAction(QString::fromStdU16String(Translate("Open page")));
    QObject::connect(open, &QAction::triggered, menu.get(),
                     [this, page_id] { page_switcher_.ActivatePage(page_id); });
    menu->setDefaultAction(open);
    menu->addSeparator();
  }

  add(ID_PAGE_RENAME, "Rename", is_current);
  add(ID_PAGE_DUPLICATE, "Duplicate", is_current);

  // Reordering acts on the right-clicked page directly, the way the icon
  // submenu does: moving a page does not require having it open, and forcing a
  // switch first would be a worse way to rearrange a list. Each end of the list
  // disables its own direction rather than silently doing nothing.
  const int index =
      entry == pages.end() ? -1 : static_cast<int>(entry - pages.begin());
  auto add_move = [&](const char* label, int target, bool enabled) {
    QAction* action =
        menu->addAction(QString::fromStdU16String(Translate(label)));
    action->setEnabled(enabled);
    QObject::connect(action, &QAction::triggered, menu.get(),
                     [this, page_id, target] { ReorderPage(page_id, target); });
  };
  add_move("Move up", index - 1, index > 0);
  add_move("Move down", index + 1,
           index >= 0 && index + 1 < static_cast<int>(pages.size()));

  // The icon is a property of the page, not of the current one, so unlike
  // Rename and Delete it acts on the right-clicked page directly — no need to
  // switch to it first, and no reason to disable it when another page is open.
  QMenu* icon_menu =
      menu->addMenu(QString::fromStdU16String(Translate("Icon")));
  const std::string current_icon = PageIconFor(page_id);

  QAction* none_action =
      icon_menu->addAction(QString::fromStdU16String(Translate("None")));
  none_action->setCheckable(true);
  none_action->setChecked(current_icon.empty());
  QObject::connect(none_action, &QAction::triggered, menu.get(),
                   [this, page_id] { SetPageIcon(page_id, {}); });
  icon_menu->addSeparator();

  for (const PageIcon& icon : GetPageIcons()) {
    QAction* action =
        icon_menu->addAction(QString::fromStdU16String(Translate(icon.label)));
    action->setCheckable(true);
    action->setChecked(current_icon == icon.key);
    const std::string key{icon.key};
    QObject::connect(action, &QAction::triggered, menu.get(),
                     [this, page_id, key] { SetPageIcon(page_id, key); });
  }

  // Delete sits last, behind its own separator: the destructive item is kept
  // away from the ones above it so it is not reached by muscle memory.
  menu->addSeparator();
  add(ID_PAGE_DELETE, "Delete page", is_current);

  menu->addSeparator();
  add(ID_PAGE_NEW, "New page", true);

  return menu;
}

void RailPagesController::ShowPageContextMenu(int page_id,
                                              const QPoint& global_pos) {
  std::unique_ptr<QMenu> menu = BuildPageContextMenu(page_id);
  menu->exec(global_pos);
}
