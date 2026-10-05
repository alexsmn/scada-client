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

// Whether a paste carries an analog item's limits into the new item. Limits
// are process data that only the Control right writes (backlog 864), so a
// paster without it passes kDrop and gets the item without its limits rather
// than a paste the server half-refuses. Ask
// SessionService::HasProcessPermission(Permission::kWrite).
enum class PasteLimits { kKeep, kDrop };

Awaitable<void> PasteNodesFromClipboard(TaskManager& task_manager,
                                        const scada::NodeId& new_parent_id,
                                        PasteLimits limits);

Awaitable<void> PasteNodesFromNodeStateRecursive(TaskManager& task_manager,
                                                 scada::NodeState&& node_state,
                                                 PasteLimits limits);

NodeRef GetPasteParentNode(NodeService& node_service,
                           CreateTree& create_tree,
                           const NodeRef& selected_node,
                           const NodeRef& root_node);
