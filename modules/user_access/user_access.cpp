#include "user_access/user_access.h"

namespace {

bool Has(scada::Permission permissions, scada::Permission wanted) {
  return (permissions & wanted) == wanted;
}

}  // namespace

scada::Permission EffectivePermissions(std::span<const AccountRole> roles) {
  scada::Permission permissions = scada::Permission::kNone;
  for (const AccountRole& role : roles) {
    // A Role the server publishes that is not one of the eight well-known ones
    // is a custom (group) Role, and its permissions are a per-namespace policy
    // (Part 3 §5.2.9) the client cannot read. Contributing nothing for it is
    // the honest choice: the alternative is to guess, and the inspector would
    // then under- or over-state what the account may do.
    if (!scada::IsWellKnownRoleId(role.node_id)) {
      continue;
    }
    for (const scada::WellKnownRole known :
         {scada::WellKnownRole::kAnonymous,
          scada::WellKnownRole::kAuthenticatedUser,
          scada::WellKnownRole::kObserver, scada::WellKnownRole::kOperator,
          scada::WellKnownRole::kEngineer, scada::WellKnownRole::kSupervisor,
          scada::WellKnownRole::kConfigureAdmin,
          scada::WellKnownRole::kSecurityAdmin}) {
      if (scada::WellKnownRoleId(known) == role.node_id) {
        permissions = permissions | scada::DefaultPermissionsForRole(known);
        break;
      }
    }
  }
  return permissions;
}

std::vector<UserPermission> PermissionsForRoles(
    std::span<const AccountRole> roles) {
  const scada::Permission permissions = EffectivePermissions(roles);
  // Each coarse capability is backed by the concrete PermissionType bits the
  // server checks for that operation, so a row cannot claim a capability the
  // server would refuse.
  return {
      {UserPermissionKind::kView,
       Has(permissions, scada::Permission::kBrowse | scada::Permission::kRead)},
      {UserPermissionKind::kControl,
       Has(permissions, scada::Permission::kWrite | scada::Permission::kCall)},
      {UserPermissionKind::kConfigure,
       Has(permissions,
           scada::Permission::kAddNode | scada::Permission::kDeleteNode)},
  };
}

const char* UserPermissionLabelKey(UserPermissionKind kind) {
  switch (kind) {
    case UserPermissionKind::kView:
      return "View & monitor";
    case UserPermissionKind::kControl:
      return "Control & manual input";
    case UserPermissionKind::kConfigure:
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
