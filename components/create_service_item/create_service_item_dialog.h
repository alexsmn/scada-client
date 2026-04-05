#pragma once

#include "scada/node_id.h"

class DialogService;
class NodeService;
class TaskManager;

struct CreateServiceItemContext {
  NodeService& node_service_;
  TaskManager& task_manager_;
  const scada::NodeId parent_id_;
};

void ShowCreateServiceItemDialog(DialogService& dialog_service,
                                 CreateServiceItemContext&& context);
