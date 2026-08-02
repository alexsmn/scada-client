#pragma once

#include "base/any_executor.h"
#include "scada/session_service.h"

template <class T>
class BasicCommandRegistry;

class LocalEvents;
class NodeService;
class Profile;
class UiCommandRegistry;
struct SelectionCommandContext;

struct ChangePasswordModuleContext {
  AnyExecutor executor_;
  LocalEvents& local_events_;
  Profile& profile_;
  scada::SessionService& session_service_;
  NodeService& node_service_;
  BasicCommandRegistry<SelectionCommandContext>& selection_commands_;
  UiCommandRegistry& ui_command_registry_;
};

class ChangePasswordModule : private ChangePasswordModuleContext {
 public:
  explicit ChangePasswordModule(ChangePasswordModuleContext&& context);
};
