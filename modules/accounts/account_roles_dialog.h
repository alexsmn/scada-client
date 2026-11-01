#pragma once

#include "scada/node_id.h"
#include "user_access/role_membership.h"

#include <functional>
#include <string>
#include <vector>

class DialogService;

// One row of the Roles dialog: a Role whose membership an administrator may
// edit, and whether the account holds it now.
struct AccountRoleChoice {
  scada::NodeId role_id;
  std::u16string name;
  bool held = false;
};

// The rows the Roles dialog offers for `account`: every editable Role in
// `roles` (IsRoleMembershipEditable), in RoleSet order, each marked with
// whether the account holds it. Anonymous and AuthenticatedUser are absent —
// the server grants them by token class and would refuse the change, so a
// checkbox for them would promise a write that cannot be made.
std::vector<AccountRoleChoice> MakeAccountRoleChoices(
    const std::vector<RoleMembership>& roles,
    std::u16string_view account);

// Shows the Roles dialog for `account`. `on_accept` receives the Roles the
// administrator left checked when the dialog is accepted; it is not called on
// Cancel.
void ShowAccountRolesDialog(
    DialogService& dialog_service,
    std::u16string account,
    std::vector<AccountRoleChoice> choices,
    std::function<void(std::vector<scada::NodeId> desired)> on_accept);
