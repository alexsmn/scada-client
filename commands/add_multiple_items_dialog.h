#pragma once

namespace scada {
class ViewService;
}

class NodeRef;
class NodeService;
class TaskManager;

void ShowAddMultipleItemsDialog(scada::ViewService& view_service,
                                NodeService& node_service,
                                const NodeRef& node,
                                TaskManager& task_manager);
