#pragma once

#include "base/any_executor.h"
#include "scada/session_service.h"

template <class T>
class BasicCommandRegistry;

class LocalEvents;
class NodeService;
class Profile;
class UiCommandRegistry;
struct GlobalCommandContext;
struct SelectionCommandContext;

struct ChangePasswordModuleContext {
  AnyExecutor executor_;
  LocalEvents& local_events_;
  Profile& profile_;
  scada::SessionService& session_service_;
  NodeService& node_service_;
  BasicCommandRegistry<SelectionCommandContext>& selection_commands_;
  // Hosts "Change Password..." for the signed-in account, which depends on no
  // selection (backlog 1098).
  BasicCommandRegistry<GlobalCommandContext>& global_commands_;
  UiCommandRegistry& ui_command_registry_;
};

// Registers the two password commands: the administrator's "Set Password..."
// on a selected account, and everyone's "Change Password..." for their own.
class ChangePasswordModule : private ChangePasswordModuleContext {
 public:
  explicit ChangePasswordModule(ChangePasswordModuleContext&& context);
};
