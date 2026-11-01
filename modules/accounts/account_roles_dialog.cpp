#include "accounts/account_roles_dialog.h"

#include <algorithm>

std::vector<AccountRoleChoice> MakeAccountRoleChoices(
    const std::vector<RoleMembership>& roles,
    std::u16string_view account) {
  std::vector<AccountRoleChoice> choices;
  for (const RoleMembership& role : roles) {
    if (!IsRoleMembershipEditable(role.node_id)) {
      continue;
    }
    choices.push_back({.role_id = role.node_id,
                       .name = role.name,
                       .held = std::ranges::find(role.members, account) !=
                               role.members.end()});
  }
  return choices;
}
