#pragma once

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
class Favourites;
class FileCache;
class LocalEvents;
class MainWindowManager;
class NodeService;
class PortfolioManager;
class Profile;
class Speech;
class TaskManager;
class TimedDataService;

struct MainWindowContext {
  ActionManager& action_manager_;
  const AliasResolver alias_resolver_;
  const int window_id_;
  events::EventManager& event_manager_;
  Favourites& favourites_;
  FileCache& file_cache_;
  LocalEvents& local_events_;
  MainWindowManager& main_window_manager_;
  NodeService& node_service_;
  PortfolioManager& portfolio_manager_;
  Profile& profile_;
  scada::HistoryService& history_service_;
  scada::MethodService& method_service_;
  scada::MonitoredItemService& monitored_item_service_;
  scada::NodeManagementService& node_management_service_;
  scada::SessionService& session_service_;
  Speech& speech_;
  TaskManager& task_manager_;
  TimedDataService& timed_data_service_;
};
