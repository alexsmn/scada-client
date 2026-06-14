#pragma once

#include "base/any_executor.h"
#include "configuration/tree/node_service_tree.h"
#include "configuration/tree/node_service_tree_impl.h"

#include <functional>
#include <vector>

template <class T>
class BasicCommandRegistry;
class ControllerRegistry;
class LocalEvents;
class NodeRef;
class Profile;
class TaskManager;
class UiCommandRegistry;
struct SelectionCommandContext;

namespace scada {
class NodeId;
class SessionService;
class Variant;
}  // namespace scada

using NodeServiceTreeFactory = std::function<std::unique_ptr<NodeServiceTree>(
    NodeServiceTreeImplContext&&)>;

struct ConfigurationModuleContext {
  AnyExecutor executor_;
  ControllerRegistry& controller_registry_;
  Profile& profile_;
  NodeServiceTreeFactory node_service_tree_factory_;
  scada::SessionService& session_service_;
  LocalEvents& local_events_;
  TaskManager& task_manager_;
  BasicCommandRegistry<SelectionCommandContext>& selection_commands_;
  UiCommandRegistry& ui_command_registry_;
};

class ConfigurationModule : private ConfigurationModuleContext {
 public:
  explicit ConfigurationModule(ConfigurationModuleContext&& context);

 private:
  void RegisterMethodCommand(unsigned command_id,
                             const scada::NodeId& method_id);
  void RegisterEnableDeviceCommand(unsigned command_id, bool enable);

  void CallMethod(const NodeRef& node,
                  const scada::NodeId& method_id,
                  const std::vector<scada::Variant>& arguments);
};
