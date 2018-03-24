#include "components/main/main_window_manager.h"

#include "services/profile.h"

#if defined(UI_VIEWS)
#include "components/main/views/main_window_views.h"
using MainWindowType = MainWindowViews;
#elif defined(UI_QT)
#include "components/main/qt/main_window_qt.h"
using MainWindowType = MainWindowQt;
#endif

MainWindowManager::MainWindowManager(MainWindowManagerContext&& context)
    : MainWindowManagerContext{std::move(context)} {
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

  auto window = CreateMainWindow(window_id);
  if (window)
    main_windows_[window_id] = std::move(window);
}

void MainWindowManager::CloseMainWindow(int window_id) {
  main_windows_.erase(window_id);

  if (main_windows_.empty())
    quit_handler_();
  else
    profile_.main_windows.erase(window_id);
}

std::unique_ptr<MainWindow> MainWindowManager::CreateMainWindow(int window_id) {
  return std::make_unique<MainWindowType>(MainWindowContext{
      action_manager_,
      alias_resolver_,
      window_id,
      event_manager_,
      favourites_,
      file_cache_,
      local_events_,
      *this,
      node_service_,
      portfolio_manager_,
      profile_,
      history_service_,
      method_service_,
      monitored_item_service_,
      node_management_service_,
      session_service_,
      speech_,
      task_manager_,
      timed_data_service_,
  });
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
    const base::FilePath& path) {
  for (auto& p : main_windows_) {
    auto view = p.second->FindOpenedViewByFilePath(path);
    if (view)
      return view;
  }
  return NULL;
}

void MainWindowManager::CreateMainWindow() {
  int window_id = profile_.CreateWindowId();
  OpenMainWindow(window_id);
}
