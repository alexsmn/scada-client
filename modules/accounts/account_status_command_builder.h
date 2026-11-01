#pragma once

#include "base/any_executor.h"
#include "controller/command_registry.h"
#include "core/selection_command_context.h"

namespace scada {
class SessionService;
}

class LocalEvents;
class NodeService;
class Profile;
class UiCommandRegistry;

// Builds "Enable Account" and "Disable Account" on a selected account (backlog
// 149): the Users view showed whether an account was enabled and offered no
// way to change it. Each flips the Disabled bit of the account's
// UserConfigurationMask through the standard ModifyUser method (see
// SetAccountDisabled), and each is offered only when it would change
// something — or both, while the account's current mask is not resident.
struct AccountStatusCommandBuilder {
  // `disable` selects which of the two commands to build.
  BasicCommand<SelectionCommandContext> Build(bool disable);

  AnyExecutor executor_;
  LocalEvents& local_events_;
  Profile& profile_;
  scada::SessionService& session_service_;
  NodeService& node_service_;
};

void RegisterAccountStatusCommandActions(
    UiCommandRegistry& ui_command_registry);
