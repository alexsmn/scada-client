#pragma once

class CommandRegistry;
class DialogService;
class MainCommands;
class NodeService;
class TaskManager;

struct ExportConfigurationModuleContext {
  NodeService& node_service_;
  TaskManager& task_manager_;
};

class ExportConfigurationModule : private ExportConfigurationModuleContext {
 public:
  explicit ExportConfigurationModule(
      ExportConfigurationModuleContext&& context);
};
