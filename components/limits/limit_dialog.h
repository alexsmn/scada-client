#pragma once

class DialogService;
class TaskManager;
class NodeRef;

void ShowLimitsDialog(DialogService& dialog_service, const NodeRef& node, TaskManager& task_manager);