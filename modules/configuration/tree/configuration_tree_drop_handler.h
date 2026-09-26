#pragma once

#include "aui/drag_drop_types.h"
#include "aui/handlers.h"
#include "base/any_executor.h"

#include <memory>

namespace scada {
class NodeId;
}

class ConfigurationTreeNode;
class CreateTree;
class NodeService;
class TaskManager;

struct ConfigurationTreeDropHandlerContext {
  AnyExecutor executor_;
  NodeService& node_service_;
  TaskManager& task_manager_;
  CreateTree& create_tree_;
};

class ConfigurationTreeDropHandler
    : private ConfigurationTreeDropHandlerContext {
 public:
  explicit ConfigurationTreeDropHandler(
      ConfigurationTreeDropHandlerContext&& context);

  // Resolves the drop of an `ItemDragData` payload onto `target_node`. A
  // payload of several nodes drops only when every node accepts the same
  // operation, and `action` then performs it for all of them.
  int GetDropAction(const DragData& drag_data,
                    const ConfigurationTreeNode* target_node,
                    DropAction& action);

  int GetDropAction(const scada::NodeId& dragging_id,
                    const ConfigurationTreeNode* target_node,
                    DropAction& action);
};
