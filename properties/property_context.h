#pragma once

class DialogService;
class NodeService;
class TaskManager;

struct PropertyContext {
  NodeService& node_service_;
  TaskManager& task_manager_;
  DialogService& dialog_service_;
};
