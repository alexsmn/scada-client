#pragma once

template <typename T>
class BasicCommandRegistry;

class CommandRegistry;
class DialogService;
class MainCommands;
class NodeService;
class TaskManager;
struct MainCommandContext;

struct ExportConfigurationModuleContext {
  NodeService& node_service_;
  TaskManager& task_manager_;
  BasicCommandRegistry<MainCommandContext>& main_commands_;
};

class ExportConfigurationModule : private ExportConfigurationModuleContext {
 public:
  explicit ExportConfigurationModule(
      ExportConfigurationModuleContext&& context);
};
