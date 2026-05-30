#pragma once

#include "base/any_executor.h"

#include <memory>

template <typename T>
class BasicCommandRegistry;

class CommandRegistry;
class DialogService;
class NodeService;
class TaskManager;
class UiCommandRegistry;
struct GlobalCommandContext;

struct ExportConfigurationModuleContext {
  AnyExecutor executor_;
  NodeService& node_service_;
  TaskManager& task_manager_;
  BasicCommandRegistry<GlobalCommandContext>& global_commands_;
  UiCommandRegistry& ui_command_registry_;
};

class ExportConfigurationModule : private ExportConfigurationModuleContext {
 public:
  explicit ExportConfigurationModule(
      ExportConfigurationModuleContext&& context);
};
