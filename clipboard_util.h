#pragma once

#include "base/promise.h"
#include "node_service/node_ref.h"

#include <vector>

class TaskManager;

void CopyNodesToClipboard(const std::vector<NodeRef>& nodes);

promise<> PasteNodesFromClipboard(TaskManager& task_manager,
                                  const scada::NodeId& new_parent_id);
