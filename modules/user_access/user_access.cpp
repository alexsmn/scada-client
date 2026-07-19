#include "user_access/user_access.h"

#include "scada/privileges.h"

namespace {

bool HasPrivilege(int access_rights, scada::Privilege privilege) {
  return (access_rights & (1 << static_cast<int>(privilege))) != 0;
}

}  // namespace

UserRole UserRoleFor(int access_rights) {
  if (HasPrivilege(access_rights, scada::Privilege::Configure))
    return UserRole::kAdministrator;
  if (HasPrivilege(access_rights, scada::Privilege::Control))
    return UserRole::kOperator;
  return UserRole::kObserver;
}

const char* UserRoleLabelKey(UserRole role) {
  switch (role) {
    case UserRole::kAdministrator:
      return "Administrator";
    case UserRole::kOperator:
      return "Operator";
    case UserRole::kObserver:
      return "Observer";
  }
  return "Observer";
}

std::vector<UserPermission> UserPermissionsFor(int access_rights) {
  return {
      {UserPermissionKind::kView, true},
      {UserPermissionKind::kControl,
       HasPrivilege(access_rights, scada::Privilege::Control)},
      {UserPermissionKind::kConfigure,
       HasPrivilege(access_rights, scada::Privilege::Configure)},
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

const char* UserSessionsLabelKey(bool multi_sessions) {
  return multi_sessions ? "Multiple" : "Single";
}
