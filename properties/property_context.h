#pragma once

#include <memory>

class DialogService;
class Executor;
class NodeService;
class TaskManager;

struct PropertyContext {
  std::shared_ptr<Executor> executor_;
  NodeService& node_service_;
  TaskManager& task_manager_;
  DialogService& dialog_service_;
};
