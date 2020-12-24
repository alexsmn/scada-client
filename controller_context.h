#pragma once

#include "common/aliases.h"

namespace scada {
class MonitoredItemService;
class HistoryService;
class SessionService;
}  // namespace scada

class BlinkerManager;
class CommandHandler;
class ContentsModel;
class ControllerDelegate;
class DialogService;
class EventFetcher;
class ExportModel;
class Favourites;
class FileCache;
class LocalEvents;
class NodeService;
class PortfolioManager;
class Profile;
class SelectionModel;
class TaskManager;
class TimedDataService;
class TimeModel;
class WindowDefinition;

struct ControllerContext {
  ControllerDelegate& controller_delegate_;
  const AliasResolver alias_resolver_;
  TaskManager& task_manager_;
  scada::SessionService& session_service_;
  EventFetcher& event_fetcher_;
  scada::HistoryService& history_service_;
  scada::MonitoredItemService& monitored_item_service_;
  TimedDataService& timed_data_service_;
  NodeService& node_service_;
  PortfolioManager& portfolio_manager_;
  LocalEvents& local_events_;
  Favourites& favourites_;
  FileCache& file_cache_;
  Profile& profile_;
  DialogService& dialog_service_;
  BlinkerManager& blinker_manager_;
};
