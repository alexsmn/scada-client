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
// revoked.
//
// This client does not know what a Role grants and must not: it READS the
// server's role → permission map off the RolePermissions attribute
// (`ReadServerRolePermissions`, OPC UA Part 3 §5.2.9) and each AccountRole
// arrives carrying its published grant. A local copy of that map is what would
// let the inspector tell an operator that an account may do something the
// server will refuse — the same class of lie the retired AccessRights bitmask
// told.

struct UserPermission {
  // The coarse capability, defined once in core (`scada::Capability`) because
  // both clients present the same three.
  scada::Capability kind;
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

// The union of the grants the server published for `roles`. A Role the server
// published no grant for contributes nothing — that is a custom (group) Role,
// whose permissions are a per-namespace policy the client cannot see (Part 3
// §5.2.9), so claiming anything about it would be invention.
scada::Permission EffectivePermissions(std::span<const AccountRole> roles);

// The capability's label key (an English literal for Translate()).
const char* UserPermissionLabelKey(scada::Capability kind);

// The session-policy label key for a Sessions cell: a user whose MultiSessions
// flag is set may hold several concurrent sessions. An unset optional means
// the flag could not be read and reads "No data" — false is a real answer
// ("single session"), so an absent read must not borrow it.
const char* UserSessionsLabelKey(std::optional<bool> multi_sessions);
