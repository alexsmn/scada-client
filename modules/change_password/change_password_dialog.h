#pragma once

#include "base/any_executor.h"

#include "node_service/node_ref.h"

#include <memory>

class DialogService;
class LocalEvents;
class Profile;

struct ChangePasswordContext {
  const NodeRef user_;
  AnyExecutor executor_;
  LocalEvents& local_events_;
  Profile& profile_;
  NodeService* node_service_ = nullptr;
  // True when the target is the CALLER'S OWN account.
  //
  // It selects both the method and the form. OPC UA Part 18 splits the two
  // cases: §5.2.8 ChangePassword applies to the user of the current session
  // and proves the old password; §5.2.7 ModifyUser is the administrator
  // resetting someone else's account and does not require it. So an admin
  // reset hides the current-password field rather than collecting a
  // credential that is neither checked nor sent.
  bool self_service_ = false;
};

// Opens the change-password form, modeless.
//
// The form reads the password policy the server publishes (OPC UA Part 18
// §5.2.2, ReadPasswordPolicy) through `node_service_` once it is open, then
// lists the rules and keeps OK disabled while the new password breaks one.
// Until the read completes, or when it fails, no rules are shown and nothing
// is checked: the server is always the judge (backlog 1100).
void ShowChangePasswordDialog(DialogService& dialog_service,
                              ChangePasswordContext&& context);
