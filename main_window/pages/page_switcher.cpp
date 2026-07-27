#include "main_window/pages/page_switcher.h"

#include "aui/dialog_service.h"
#include "aui/translation.h"
#include "base/awaitable.h"
#include "base/u16format.h"
#include "main_window/main_window_interface.h"
#include "main_window/main_window_manager.h"
#include "profile/profile.h"

#include <algorithm>
#include <ranges>

PageSwitcher::PageSwitcher(PageSwitcherContext&& context)
    : PageSwitcherContext{std::move(context)} {}

PageSwitcher::~PageSwitcher() {}

std::vector<PageEntry> PageSwitcher::ListPages() const {
  const int current_page_id = main_window_.GetCurrentPage().id;

  std::vector<PageEntry> entries;
  entries.reserve(profile_.pages.size());
  for (const auto& [page_id, page] : profile_.pages) {
    const bool current = page_id == current_page_id;
    entries.push_back(PageEntry{
        // GetTitle() rather than the raw `title` field: a page saved without
        // an explicit title still has to name itself in the list.
        .page_id = page_id,
        .title = page.GetTitle(),
        .current = current,
        .opened_elsewhere =
            !current && main_window_manager_.IsPageOpened(page_id),
        .order = page.order});
  }

  // Unordered pages (order == 0) sort last, by id, so a profile written before
  // reordering existed keeps the order it always had.
  std::ranges::stable_sort(entries, [](const PageEntry& a, const PageEntry& b) {
    const bool a_ordered = a.order != 0;
    const bool b_ordered = b.order != 0;
    if (a_ordered != b_ordered)
      return a_ordered;
    if (a.order != b.order)
      return a.order < b.order;
    return a.page_id < b.page_id;
  });

  return entries;
}

void PageSwitcher::ReorderPage(int page_id, int new_index) {
  std::vector<PageEntry> entries = ListPages();

  const auto moved = std::ranges::find(entries, page_id, &PageEntry::page_id);
  if (moved == entries.end())
    return;

  const int old_index = static_cast<int>(moved - entries.begin());
  new_index = std::clamp(new_index, 0, static_cast<int>(entries.size()) - 1);
  if (new_index == old_index)
    return;

  const PageEntry entry = *moved;
  entries.erase(moved);
  entries.insert(entries.begin() + new_index, entry);

  // Renumber densely from 1: order 0 means "unordered", so every page has to
  // pick up a real position once the operator has expressed one.
  for (std::size_t index = 0; index < entries.size(); ++index) {
    auto page = profile_.pages.find(entries[index].page_id);
    if (page != profile_.pages.end())
      page->second.order = static_cast<int>(index) + 1;
  }

  profile_.NotifyChange();
}

void PageSwitcher::ActivatePage(int page_id) {
  auto i = profile_.pages.find(page_id);
  if (i == profile_.pages.end())
    return;

  const Page& page = i->second;
  const Page& current_page = main_window_.GetCurrentPage();

  const bool revert = page.id == current_page.id;
  if (!revert) {
    OpenPageHelper(page, false);
    return;
  }

  // Re-activating the open page means "throw away my unsaved layout changes",
  // which is destructive enough to confirm.
  std::u16string message =
      u16format(L"Return to saved page {}?", current_page.GetTitle());
  CoSpawn(executor_, cancelation_,
          [this, page_ptr = &page,
           message = std::move(message)]() -> Awaitable<void> {
            auto message_box_result = co_await dialog_service_.RunMessageBox(
                message, {}, MessageBoxMode::QuestionYesNo);
            if (message_box_result == MessageBoxResult::Yes)
              OpenPageHelper(*page_ptr, true);
            co_return;
          });
}

void PageSwitcher::OpenPageHelper(const Page& page, bool revert) {
  // Don't allow to open same page in different windows.
  if (!revert && main_window_manager_.IsPageOpened(page.id)) {
    dialog_service_.RunMessageBox(
        Translate("The specified page is open in another window."), {},
        MessageBoxMode::Info);
    return;
  }

  if (!revert) {
    main_window_.SaveCurrentPage();
  }

  main_window_.OpenPage(page);
}
