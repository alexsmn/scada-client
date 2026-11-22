#pragma once

#include "base/any_executor.h"
#include "controller/command_registry.h"
#include "core/selection_command_context.h"

#include <functional>
#include <memory>

namespace scada {
class SessionService;
}

class DialogService;
class LocalEvents;
class NodeService;
class Profile;
class UiCommandRegistry;
struct ChangePasswordContext;
struct GlobalCommandContext;

// "Set Password..." on a selected user node: an administrator resetting the
// account (or, when the selected node is the caller's own, a self change).
// Offered only with the Configure right.
struct ChangePasswordCommandBuilder {
  BasicCommand<SelectionCommandContext> Build();

  AnyExecutor executor_;
  LocalEvents& local_events_;
  Profile& profile_;
  scada::SessionService& session_service_;
  // The standard methods live on the UserManagement object, so the command
  // needs the service to resolve it (the per-user node no longer carries them).
  NodeService& node_service_;
};

// "Change Password..." for the signed-in account itself, offered to every
// authenticated session whatever its rights and whatever is selected (backlog
// 1098). It always takes the self-service path -- OPC UA Part 18 §5.2.8
// ChangePassword, which proves the current password -- so the Server needs no
// administrative right to grant it, and a 2.5.7 Server, which changes a
// password only together with the current one, accepts it too.
struct ChangeOwnPasswordCommandBuilder {
  // Opens the form; ShowChangePasswordDialog unless a test substitutes one.
  using ShowDialog =
      std::function<void(DialogService&, ChangePasswordContext&&)>;

  BasicCommand<GlobalCommandContext> Build();

  AnyExecutor executor_;
  LocalEvents& local_events_;
  Profile& profile_;
  scada::SessionService& session_service_;
  NodeService& node_service_;
  ShowDialog show_dialog_;
};

// Whether `session_service` is signed in to an account that has a password of
// its own to change: connected, with a user id, and not anonymous.
bool CanChangeOwnPassword(const scada::SessionService& session_service);

// Registers the titles of both commands and places "Change Password..." in the
// main menu's account slot.
void RegisterChangePasswordCommandActions(
    UiCommandRegistry& ui_command_registry);
