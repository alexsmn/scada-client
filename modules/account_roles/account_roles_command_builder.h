#pragma once

#include "base/any_executor.h"
#include "controller/command_registry.h"
#include "core/selection_command_context.h"

namespace scada {
class AttributeService;
class SessionService;
}  // namespace scada

class LocalEvents;
class NodeService;
class Profile;
class UiCommandRegistry;

// Builds "Roles..." on a selected account: read the RoleSet, let an
// administrator check the Roles the account should hold, and grant or revoke
// the difference (AddIdentity / RemoveIdentity, OPC UA Part 18 §4.4.5/§4.4.6).
//
// Role membership is the whole authorization model — an account's rights at
// sign-in are derived from the Roles it holds and nothing else — so until this
// command an account created in the Client could only look (backlog 937).
struct AccountRolesCommandBuilder {
  BasicCommand<SelectionCommandContext> Build();

  AnyExecutor executor_;
  LocalEvents& local_events_;
  Profile& profile_;
  scada::SessionService& session_service_;
  NodeService& node_service_;
  // Reads what each Role grants (ReadRoleMemberships needs the server's
  // role -> permission map to answer at all).
  scada::AttributeService& attribute_service_;
};

void RegisterAccountRolesCommandActions(UiCommandRegistry& ui_command_registry);
