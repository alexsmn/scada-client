#pragma once

#include "controls/handlers.h"
#include "ui/base/dragdrop/drag_drop_types.h"

namespace scada {
class NodeId;
}

class ConfigurationTreeNode;
class NodeService;
class TaskManager;

struct ConfigurationTreeDropHandlerContext {
  NodeService& node_service_;
  TaskManager& task_manager_;
};

class ConfigurationTreeDropHandler
    : private ConfigurationTreeDropHandlerContext {
 public:
  explicit ConfigurationTreeDropHandler(
      ConfigurationTreeDropHandlerContext&& context);

  int GetDropAction(const DragData& drag_data,
                    ConfigurationTreeNode* target_node,
                    DropAction& action);

  int GetDropAction(const scada::NodeId& dragging_id,
                    ConfigurationTreeNode* target_node,
                    DropAction& action);
};
