#pragma once

#include <memory>

template <typename T>
class BasicCommandRegistry;

class CommandRegistry;
class DialogService;
class Executor;
class NodeService;
class TaskManager;
struct GlobalCommandContext;

struct ExportConfigurationModuleContext {
  std::shared_ptr<Executor> executor_;
  NodeService& node_service_;
  TaskManager& task_manager_;
  BasicCommandRegistry<GlobalCommandContext>& global_commands_;
};

class ExportConfigurationModule : private ExportConfigurationModuleContext {
 public:
  explicit ExportConfigurationModule(
      ExportConfigurationModuleContext&& context);
};
