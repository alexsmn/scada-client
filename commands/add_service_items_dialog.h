#pragma once

#include "core/node_id.h"

class NodeService;
class TaskManager;

void ShowAddServiceItemsDialog(NodeService& node_service,
                               TaskManager& task_manager,
                               const scada::NodeId& node_id);
