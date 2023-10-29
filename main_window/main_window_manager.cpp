#include "main_window/main_window_manager.h"

#include "main_window/main_window.h"
#include "profile/profile.h"

MainWindowManager::MainWindowManager(MainWindowManagerContext&& context)
    : MainWindowManagerContext{std::move(context)} {}

void MainWindowManager::Init() {
  typedef Profile::MainWindows Windows;
  const Windows& window_defs = profile_.main_windows;
  if (window_defs.empty()) {
    auto window_id = profile_.CreateWindowId();
    OpenMainWindow(window_id);
  } else {
    for (auto& p : window_defs)
      OpenMainWindow(p.second.id);
  }
}

MainWindowManager::~MainWindowManager() {}

void MainWindowManager::OpenMainWindow(int window_id) {
  if (main_windows_.find(window_id) != main_windows_.end())
    return;

  if (auto window = main_window_factory_(window_id))
    main_windows_[window_id] = std::move(window);
}

void MainWindowManager::CloseMainWindow(int window_id) {
  main_windows_.erase(window_id);

  if (main_windows_.empty())
    quit_handler_();
  else
    profile_.main_windows.erase(window_id);
}

bool MainWindowManager::IsPageOpened(int page_id) const {
  for (auto& p : main_windows_) {
    auto* page = p.second->GetCurrentPage();
    if (page && page->id == page_id)
      return true;
  }
  return false;
}

Page* MainWindowManager::FindFirstNotOpenedPage() {
  Profile::PageMap& pages = profile_.pages;
  for (Profile::PageMap::iterator i = pages.begin(); i != pages.end(); ++i) {
    if (!IsPageOpened(i->second.id))
      return &i->second;
  }
  return NULL;
}

OpenedView* MainWindowManager::FindOpenedViewByFilePath(
    const std::filesystem::path& path) {
  for (auto& p : main_windows_) {
    if (auto* view = p.second->FindOpenedViewByFilePath(path))
      return view;
  }
  return NULL;
}

void MainWindowManager::CreateMainWindow() {
  int window_id = profile_.CreateWindowId();
  OpenMainWindow(window_id);
}

void MainWindowManager::OnMainWindowClosed(int window_id) {
  auto i = main_windows_.find(window_id);
  assert(i != main_windows_.end());
  if (i == main_windows_.end())
    return;

  i->second.release();
  main_windows_.erase(i);

  if (main_windows_.empty()) {
    quit_handler_();

  } else {
    profile_.main_windows.erase(window_id);
  }
}