#pragma once

class NodeRef;
class NodeRefService;
class TaskManager;

void ShowAddMultipleItemsDialog(NodeRefService& node_service, const NodeRef& node, TaskManager& task_manager);
