#pragma once

#include "common/aliases.h"

#include <map>
#include <memory>

namespace base {
class FilePath;
}

namespace events {
class EventManager;
}

namespace scada {
class HistoryService;
class MethodService;
class MonitoredItemService;
class NodeManagementService;
class SessionService;
}  // namespace scada

class ActionManager;
class MainWindow;
class OpenedView;
class Page;
class Favourites;
class FileCache;
class LocalEvents;
class NodeService;
class PortfolioManager;
class Profile;
class Speech;
class TaskManager;
class TimedDataService;

struct MainWindowManagerContext {
  const AliasResolver alias_resolver_;
  TaskManager& task_manager_;
  scada::MethodService& method_service_;
  scada::SessionService& session_service_;
  scada::NodeManagementService& node_management_service_;
  events::EventManager& event_manager_;
  scada::HistoryService& history_service_;
  scada::MonitoredItemService& monitored_item_service_;
  TimedDataService& timed_data_service_;
  PortfolioManager& portfolio_manager_;
  NodeService& node_service_;
  ActionManager& action_manager_;
  LocalEvents& local_events_;
  Favourites& favourites_;
  FileCache& file_cache_;
  Speech& speech_;
  Profile& profile_;
  const std::function<void()> quit_handler_;
};

class MainWindowManager : private MainWindowManagerContext {
 public:
  explicit MainWindowManager(MainWindowManagerContext&& context);
  ~MainWindowManager();

  void CreateMainWindow();
  void OpenMainWindow(int window_id);
  void CloseMainWindow(int window_id);

  typedef std::map<int /*window_id*/, std::unique_ptr<MainWindow>> MainWindows;
  const MainWindows& main_windows() const { return main_windows_; }

  bool IsPageOpened(int page_id) const;
  Page* FindFirstNotOpenedPage();
  OpenedView* FindOpenedViewByFilePath(const base::FilePath& path);

 private:
  std::unique_ptr<MainWindow> CreateMainWindow(int window_id);

  MainWindows main_windows_;
};
