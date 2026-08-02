#include "user_access/user_access.h"

scada::Permission EffectivePermissions(std::span<const AccountRole> roles) {
  // A plain union of what the SERVER published for each held Role (OPC UA
  // Part 3 §4.9: effective permissions are the OR across the roles a caller
  // holds). There is no map here to go stale — a Role carries its own grant,
  // read from the server's RolePermissions.
  //
  // A Role with no published grant contributes nothing. That is a custom
  // (group) Role, whose permissions are a per-namespace policy (§5.2.9) this
  // client cannot read; guessing would make the inspector over- or understate
  // what the account may do.
  scada::Permission permissions = scada::Permission::kNone;
  for (const AccountRole& role : roles) {
    if (role.permissions) {
      permissions |= *role.permissions;
    }
  }
  return permissions;
}

std::vector<UserPermission> PermissionsForRoles(
    std::span<const AccountRole> roles) {
  const scada::Permission permissions = EffectivePermissions(roles);
  // Each coarse capability is backed by the concrete PermissionType bits the
  // server checks for that operation (scada::RequiredPermissions), so a row
  // cannot claim a capability the server would refuse.
  std::vector<UserPermission> result;
  for (const scada::Capability capability :
       {scada::Capability::kView, scada::Capability::kControl,
        scada::Capability::kConfigure}) {
    result.push_back(
        UserPermission{capability, scada::Grants(permissions, capability)});
  }
  return result;
}

const char* UserPermissionLabelKey(scada::Capability kind) {
  switch (kind) {
    case scada::Capability::kView:
      return "View & monitor";
    case scada::Capability::kControl:
      return "Control & manual input";
    case scada::Capability::kConfigure:
      return "Configure & administer";
  }
  return "";
}

const char* UserSessionsLabelKey(std::optional<bool> multi_sessions) {
  // "No data" rather than a default: false is a real answer (single session),
  // so an absent read must not borrow it.
  if (!multi_sessions)
    return "No data";
  return *multi_sessions ? "Multiple" : "Single";
}
