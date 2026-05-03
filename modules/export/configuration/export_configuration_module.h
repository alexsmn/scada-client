#pragma once

#include "base/any_executor.h"

#include <memory>

class ActionManager;
class DialogService;
class NodeService;
class TaskManager;
struct GlobalCommandContext;

struct ExportConfigurationModuleContext {
  AnyExecutor executor_;
  NodeService& node_service_;
  TaskManager& task_manager_;
  ActionManager& action_manager_;
};

class ExportConfigurationModule : private ExportConfigurationModuleContext {
 public:
  explicit ExportConfigurationModule(
      ExportConfigurationModuleContext&& context);
};
