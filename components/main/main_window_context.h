#pragma once

#include "controller_factory.h"

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
class MainWindow;
class MainWindowManager;
class NodeService;
class OpenedView;
class PortfolioManager;
class Profile;
class Speech;
class StatusBarModel;
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
  const ControllerFactory controller_factory_;
  const std::function<std::unique_ptr<CommandHandler>(
      MainWindow& main_window,
      DialogService& dialog_service)>
      main_commands_factory_;
  const std::function<std::unique_ptr<CommandHandler>(
      OpenedView& opened_view,
      DialogService& dialog_service)>
      view_commands_factory_;
  const std::shared_ptr<StatusBarModel> status_bar_model_;
};
