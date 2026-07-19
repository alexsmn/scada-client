#pragma once

#include <vector>

// The access-rights model for the reshell users-admin RBAC inspector
// (users-admin.html). The server grants users two privilege bits — Configure
// and Control (scada::Privilege) — in the AccessRights bitmask; everyone may
// view. These pure helpers derive the coarse role and the permission breakdown
// from that bitmask, so they are unit-testable without Qt or a node service.

// The coarse role tier, mirroring the status strip's UserRoleKey: Configure ⇒
// Administrator, Control ⇒ Operator, otherwise Observer.
enum class UserRole { kAdministrator, kOperator, kObserver };

UserRole UserRoleFor(int access_rights);

// The role's label key (an English literal for Translate()).
const char* UserRoleLabelKey(UserRole role);

// A permission shown in the RBAC inspector.
enum class UserPermissionKind {
  kView,       // browse / live values / trends — always granted.
  kControl,    // issue commands, write values — scada::Privilege::Control.
  kConfigure,  // edit hardware / limits / users — scada::Privilege::Configure.
};

struct UserPermission {
  UserPermissionKind kind;
  bool granted;
};

// The permission breakdown for an AccessRights bitmask, in display order.
std::vector<UserPermission> UserPermissionsFor(int access_rights);

// The permission's label key (an English literal for Translate()).
const char* UserPermissionLabelKey(UserPermissionKind kind);
