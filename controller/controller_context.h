#pragma once

namespace scada {
class MonitoredItemService;
class HistoryService;
class SessionService;
}  // namespace scada

class BlinkerManager;
class ControllerDelegate;
class CreateTree;
class DialogService;
class NodeEventProvider;
class Executor;
class ExportModel;
class FileCache;
class NodeService;
class Profile;
class PropertyService;
class TaskManager;
class TimedDataService;

struct ControllerContext {
  const std::shared_ptr<Executor> executor_;
  ControllerDelegate& controller_delegate_;
  TaskManager& task_manager_;
  scada::SessionService& session_service_;
  NodeEventProvider& node_event_provider_;
  scada::HistoryService& history_service_;
  scada::MonitoredItemService& monitored_item_service_;
  TimedDataService& timed_data_service_;
  NodeService& node_service_;
  FileCache& file_cache_;
  Profile& profile_;
  DialogService& dialog_service_;
  BlinkerManager& blinker_manager_;
  CreateTree& create_tree_;
  PropertyService& property_service_;
};
