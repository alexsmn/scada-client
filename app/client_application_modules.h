#pragma once

#include "common/aliases.h"
#include "configuration/tree/node_service_tree.h"
#include "configuration/tree/node_service_tree_impl.h"
#include "core/progress_host.h"
#include "scada/services.h"

#include <functional>
#include <memory>
#include <stack>

template <class T>
class BasicCommandRegistry;

class BlinkerManager;
class ControllerRegistry;
class Executor;
class FileSystemComponent;
class NodeService;
class Profile;
class PrintModule;
class TaskManager;
class TimedDataService;
class WriteService;
struct GlobalCommandContext;
struct SelectionCommandContext;

using NodeServiceTreeFactory = std::function<
    std::unique_ptr<NodeServiceTree>(NodeServiceTreeImplContext&&)>;

struct ClientApplicationModules {
  bool configuration = true;
  bool debugger = true;
  bool modus = true;
  bool vidicon = true;
  bool opcua_services = true;
  bool print = true;
  bool export_configuration = true;
  bool node_service_progress_tracker = true;
};

struct ClientApplicationModuleContext {
  std::shared_ptr<Executor> executor_;
  scada::services scada_services_;
  AliasResolver alias_resolver_;
  ControllerRegistry& controller_registry_;
  Profile& profile_;
  NodeService& node_service_;
  TaskManager& task_manager_;
  TimedDataService& timed_data_service_;
  WriteService& write_service_;
  std::unique_ptr<PrintModule>& print_module_;
  NodeServiceTreeFactory node_service_tree_factory_;
  FileSystemComponent& filesystem_component_;
  BlinkerManager& blinker_manager_;
  ProgressHost& progress_host_;
  BasicCommandRegistry<GlobalCommandContext>& global_commands_;
  BasicCommandRegistry<SelectionCommandContext>& selection_commands_;
  std::stack<std::shared_ptr<void>>& singletons_;
};

using ClientApplicationModuleConfigurator =
    std::function<void(ClientApplicationModuleContext&)>;

ClientApplicationModuleConfigurator MakeDefaultClientApplicationModules(
    ClientApplicationModules modules = {});
