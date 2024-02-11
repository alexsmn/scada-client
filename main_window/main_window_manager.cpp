#include "main_window/main_window_manager.h"

#include "main_window/main_window.h"
#include "profile/profile.h"

#include <ranges>

MainWindowManager::MainWindowManager(MainWindowManagerContext&& context)
    : MainWindowManagerContext{std::move(context)} {}

void MainWindowManager::Init() {
  const auto& window_defs = profile_.main_windows;
  if (window_defs.empty()) {
    auto window_id = profile_.CreateWindowId();
    OpenMainWindow(window_id);
  } else {
    for (const auto& [_, window_def] : window_defs) {
      OpenMainWindow(window_def.id);
    }
  }
}

MainWindowManager::~MainWindowManager() {}

void MainWindowManager::OpenMainWindow(int window_id) {
  if (main_windows_.contains(window_id)) {
    return;
  }

  if (auto window = main_window_factory_(window_id)) {
    main_windows_.try_emplace(window_id, std::move(window));
  }
}

void MainWindowManager::CloseMainWindow(int window_id) {
  main_windows_.erase(window_id);

  if (main_windows_.empty())
    quit_handler_();
  else
    profile_.main_windows.erase(window_id);
}

bool MainWindowManager::IsPageOpened(int page_id) const {
  return std::ranges::any_of(main_windows_ | std::views::values,
                             [page_id](const auto& main_window) {
                               const auto* page = main_window->GetCurrentPage();
                               return page && page->id == page_id;
                             });
}

Page* MainWindowManager::FindFirstNotOpenedPage() {
  const auto& pages = profile_.pages | std::views::values;
  auto i = std::ranges::find_if(
      pages, [this](const Page& page) { return IsPageOpened(page.id); });
  return i != pages.end() ? &*i : nullptr;
}

OpenedView* MainWindowManager::FindOpenedViewByFilePath(
    const std::filesystem::path& path) {
  for (const auto& main_window : main_windows_ | std::views::values) {
    if (auto* view = main_window->FindOpenedViewByFilePath(path)) {
      return view;
    }
  }
  return nullptr;
}

void MainWindowManager::CreateMainWindow() {
  int window_id = profile_.CreateWindowId();
  OpenMainWindow(window_id);
}

void MainWindowManager::OnMainWindowClosed(int window_id) {
  auto i = main_windows_.find(window_id);
  assert(i != main_windows_.end());
  if (i == main_windows_.end()) {
    return;
  }

  i->second.release();
  main_windows_.erase(i);

  if (main_windows_.empty()) {
    quit_handler_();

  } else {
    profile_.main_windows.erase(window_id);
  }
}