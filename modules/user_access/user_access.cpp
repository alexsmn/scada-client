#include "user_access/user_access.h"

#include "scada/access_rights.h"

#include <cstdint>

namespace {

bool Granted(int access_rights, scada::AccessRight right) {
  return scada::HasAccessRight(static_cast<std::uint32_t>(access_rights),
                               right);
}

}  // namespace

UserRole UserRoleFor(int access_rights) {
  if (Granted(access_rights, scada::AccessRight::kConfigure))
    return UserRole::kAdministrator;
  if (Granted(access_rights, scada::AccessRight::kControl))
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
    case UserRole::kUnknown:
      // Same wording as the Inspector's third quality band, and already
      // translated ("Нет данных") — this is the same "nothing was delivered"
      // reading, not a role the server can grant.
      return "No data";
  }
  return "Observer";
}

std::vector<UserPermission> UserPermissionsFor(int access_rights) {
  return {
      {UserPermissionKind::kView, true},
      {UserPermissionKind::kControl,
       Granted(access_rights, scada::AccessRight::kControl)},
      {UserPermissionKind::kConfigure,
       Granted(access_rights, scada::AccessRight::kConfigure)},
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
  // Same wording as the unresolved role, for the same reason.
  if (!multi_sessions)
    return "No data";
  return *multi_sessions ? "Multiple" : "Single";
}
