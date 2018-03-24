#pragma once

#include "core/node_id.h"

class NodeService;
class TaskManager;

void ShowAddMultipleItemsDialog(NodeService& node_service,
                                TaskManager& task_manager,
                                const scada::NodeId& node_id);
