#pragma once

#include "scada/authorization.h"
#include "user_access/role_membership.h"

#include <optional>
#include <span>
#include <vector>

// The access-rights model for the users-admin RBAC inspector
// (users-admin.html).
//
// Authorization is ROLE-based: a session's rights come from the Roles its
// identity mapping rules grant (OPC UA Part 18 §4.4.1), and the two-bit
// `UserType.AccessRights` mask this file used to read is vestigial — the
// server does not consult it, so a client that still derived a role from it
// would keep showing "Administrator" for an account whose Role had been
// revoked. These helpers derive the permission breakdown from the granted
// Roles instead, through the SAME default role→permission map the server
// enforces with (`scada::DefaultPermissionsForRole`), so the inspector and
// the server cannot disagree about what an account may do.

// A permission shown in the RBAC inspector. These are the coarse capabilities
// an operator reasons about, each backed by a concrete OPC UA PermissionType
// bit rather than by a client-invented tier.
enum class UserPermissionKind {
  kView,       // Browse + Read: see the address space and live values.
  kControl,    // Write + Call: issue commands and write values.
  kConfigure,  // AddNode + DeleteNode: change the configuration.
};

struct UserPermission {
  UserPermissionKind kind;
  bool granted;
};

// The permission breakdown implied by the Roles an account holds, in display
// order.
//
// An account holding NO Role yields every permission ungranted, which is
// correct and not a guess: without a Role the server grants nothing beyond
// what an anonymous session gets.
std::vector<UserPermission> PermissionsForRoles(
    std::span<const AccountRole> roles);

// The union of the default permissions of `roles`. Roles the server publishes
// that are not well-known contribute nothing here — their permissions are a
// per-namespace policy the client cannot see (Part 3 §5.2.9), so claiming
// anything about them would be invention.
scada::Permission EffectivePermissions(std::span<const AccountRole> roles);

// The permission's label key (an English literal for Translate()).
const char* UserPermissionLabelKey(UserPermissionKind kind);

// The session-policy label key for a Sessions cell: a user whose MultiSessions
// flag is set may hold several concurrent sessions. An unset optional means
// the flag could not be read and reads "No data" — false is a real answer
// ("single session"), so an absent read must not borrow it.
const char* UserSessionsLabelKey(std::optional<bool> multi_sessions);
