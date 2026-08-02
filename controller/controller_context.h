#pragma once

#include "base/any_executor.h"

namespace scada {
class AttributeService;
class MonitoredItemService;
class HistoryService;
class SessionService;
}  // namespace scada

class BlinkerManager;
class ControllerDelegate;
class CreateTree;
class DialogService;
class NodeEventProvider;
class ExportModel;
class FileCache;
class FrameCaptureRegistry;
class NodeService;
class Profile;
class PropertyService;
class TaskManager;
class TimedDataService;

struct ControllerContext {
  const AnyExecutor executor_;
  ControllerDelegate& controller_delegate_;
  TaskManager& task_manager_;
  scada::SessionService& session_service_;
  NodeEventProvider& node_event_provider_;
  scada::HistoryService& history_service_;
  scada::MonitoredItemService& monitored_item_service_;
  TimedDataService& timed_data_service_;
  NodeService& node_service_;
  // Raw attribute reads, for the attributes `NodeService` does not fetch. Its
  // per-node fetch covers BrowseName/DisplayName/NodeClass/DataType/Value
  // only, so anything else — RolePermissions, for one — has to be read
  // directly rather than added to the cost of every node fetch.
  scada::AttributeService& attribute_service_;
  FileCache& file_cache_;
  Profile& profile_;
  DialogService& dialog_service_;
  BlinkerManager& blinker_manager_;
  CreateTree& create_tree_;
  PropertyService& property_service_;
  FrameCaptureRegistry& frame_capture_registry_;
};
