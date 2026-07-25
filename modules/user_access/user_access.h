#pragma once

#include <optional>
#include <vector>

// The access-rights model for the reshell users-admin RBAC inspector
// (users-admin.html). The server grants users two access-right bits — Configure
// and Control (scada::AccessRight) — in the AccessRights bitmask; everyone may
// view. These pure helpers derive the coarse role and the permission breakdown
// from that bitmask, so they are unit-testable without Qt or a node service.

// The coarse role tier, mirroring the status strip's UserRoleKey: Configure ⇒
// Administrator, Control ⇒ Operator, otherwise Observer.
//
// kUnknown is the "AccessRights was never delivered" tier, and is never
// produced by UserRoleFor — only by a caller that could not read the bitmask at
// all. It exists because an absent bitmask reads as zero, and zero is a
// perfectly valid bitmask meaning Observer-with-view-only. Rendering an
// unresolved read as a real role is the failure mode client/docs/ux/
// principles.md §5 forbids; it is the same defect the Inspector's kUnknown
// quality band was added for.
enum class UserRole { kAdministrator, kOperator, kObserver, kUnknown };

UserRole UserRoleFor(int access_rights);

// The role's label key (an English literal for Translate()).
const char* UserRoleLabelKey(UserRole role);

// A permission shown in the RBAC inspector.
enum class UserPermissionKind {
  kView,       // browse / live values / trends — always granted.
  kControl,    // issue commands, write values — scada::AccessRight::kControl.
  kConfigure,  // edit hardware / limits / users —
               // scada::AccessRight::kConfigure.
};

struct UserPermission {
  UserPermissionKind kind;
  bool granted;
};

// The permission breakdown for an AccessRights bitmask, in display order.
std::vector<UserPermission> UserPermissionsFor(int access_rights);

// The permission's label key (an English literal for Translate()).
const char* UserPermissionLabelKey(UserPermissionKind kind);

// The session-policy label key for the users grid's Sessions column: a user
// whose MultiSessions flag is set may hold several concurrent sessions. An
// unset optional means the flag could not be read and reads "No data" — false
// is a real answer ("single session"), so an absent read must not borrow it.
const char* UserSessionsLabelKey(std::optional<bool> multi_sessions);
