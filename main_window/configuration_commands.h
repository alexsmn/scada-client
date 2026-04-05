#pragma once

#include <memory>
#include <vector>

namespace scada {
class NodeId;
class SessionService;
class Variant;
}  // namespace scada

template <class T>
class BasicCommandRegistry;

class Executor;
class LocalEvents;
class NodeRef;
class Profile;
class TaskManager;
class TimedDataService;
struct SelectionCommandContext;

class ConfigurationCommands {
 public:
  void Register();

  BasicCommandRegistry<SelectionCommandContext>& selection_commands_;
  std::shared_ptr<Executor> executor_;
  TimedDataService& timed_data_service_;
  scada::SessionService& session_service_;
  Profile& profile_;
  LocalEvents& local_events_;
  TaskManager& task_manager_;

 private:
  void RegisterMethodCommand(unsigned command_id,
                             const scada::NodeId& method_id);
  void RegisterEnableDeviceCommand(unsigned command_id, bool enable);

  void CallMethod(const NodeRef& node,
                  const scada::NodeId& method_id,
                  const std::vector<scada::Variant>& arguments);
};