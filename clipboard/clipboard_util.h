#pragma once

#include "base/promise.h"
#include "node_service/node_ref.h"

#include <vector>

namespace scada {
struct NodeState;
}

class CreateTree;
class NodeService;
class TaskManager;

void CopyNodesToClipboard(const std::vector<NodeRef>& nodes);

promise<> PasteNodesFromClipboard(TaskManager& task_manager,
                                  const scada::NodeId& new_parent_id);

promise<> PasteNodesFromNodeStateRecursive(TaskManager& task_manager,
                                           scada::NodeState&& node_state);

NodeRef GetPasteParentNode(NodeService& node_service,
                           CreateTree& create_tree,
                           const NodeRef& selected_node,
                           const NodeRef& root_node);
