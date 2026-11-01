#pragma once

#include "base/any_executor.h"

template <class T>
class BasicCommandRegistry;

namespace scada {
class AttributeService;
class SessionService;
}  // namespace scada

class LocalEvents;
class NodeService;
class Profile;
class UiCommandRegistry;
struct SelectionCommandContext;

struct AccountRolesModuleContext {
  AnyExecutor executor_;
  LocalEvents& local_events_;
  Profile& profile_;
  scada::SessionService& session_service_;
  NodeService& node_service_;
  scada::AttributeService& attribute_service_;
  BasicCommandRegistry<SelectionCommandContext>& selection_commands_;
  UiCommandRegistry& ui_command_registry_;
};

// Registers the "Roles..." selection command on accounts (see
// AccountRolesCommandBuilder).
class AccountRolesModule : private AccountRolesModuleContext {
 public:
  explicit AccountRolesModule(AccountRolesModuleContext&& context);
};
