#pragma once

#include "base/any_executor.h"

#include <memory>

class DialogService;
class NodeService;
class TaskManager;

struct PropertyContext {
  AnyExecutor executor_;
  NodeService& node_service_;
  TaskManager& task_manager_;
  DialogService& dialog_service_;
};
