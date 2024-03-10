#pragma once

#include "aui/drag_drop_types.h"
#include "aui/handlers.h"

namespace scada {
class NodeId;
}

class ConfigurationTreeNode;
class CreateTree;
class NodeService;
class TaskManager;

struct ConfigurationTreeDropHandlerContext {
  NodeService& node_service_;
  TaskManager& task_manager_;
  CreateTree& create_tree_;
};

class ConfigurationTreeDropHandler
    : private ConfigurationTreeDropHandlerContext {
 public:
  explicit ConfigurationTreeDropHandler(
      ConfigurationTreeDropHandlerContext&& context);

  int GetDropAction(const DragData& drag_data,
                    const ConfigurationTreeNode* target_node,
                    DropAction& action);

  int GetDropAction(const scada::NodeId& dragging_id,
                    const ConfigurationTreeNode* target_node,
                    DropAction& action);
};
