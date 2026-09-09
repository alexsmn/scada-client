#pragma once

#include "base/any_executor.h"
#include "base/awaitable.h"
#include "node_service/node_ref.h"

#include <vector>

namespace scada {
struct NodeState;
}

class CreateTree;
class NodeService;
class TaskManager;

// Serializes `nodes` and their subtrees to the OS clipboard. `executor` must
// be the GUI executor: the walk reads the NodeService cache, which is
// executor-affine (node_service.h), before and after each suspension.
void CopyNodesToClipboard(AnyExecutor executor,
                          const std::vector<NodeRef>& nodes);

Awaitable<void> PasteNodesFromClipboard(TaskManager& task_manager,
                                        const scada::NodeId& new_parent_id);

Awaitable<void> PasteNodesFromNodeStateRecursive(TaskManager& task_manager,
                                                 scada::NodeState&& node_state);

NodeRef GetPasteParentNode(NodeService& node_service,
                           CreateTree& create_tree,
                           const NodeRef& selected_node,
                           const NodeRef& root_node);
