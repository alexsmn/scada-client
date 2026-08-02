#pragma once

#include "base/any_executor.h"
#include "base/awaitable.h"
#include "scada/authorization.h"
#include "scada/node_id.h"

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace scada {
class AttributeService;
}

class NodeService;

// One Role of `Server.ServerCapabilities.RoleSet`, with the accounts it grants
// itself to.
//
// Role membership IS this server's authorization model (OPC UA Part 18
// §4.4.1): a session's rights are derived from the Roles its identity mapping
// rules grant, not from the retired access-rights bitmask. So this is the
// shared source both admin surfaces read — the Users grid's Roles column and
// the Roles view — rather than each browsing the RoleSet its own way.
struct RoleMembership {
  scada::NodeId node_id;
  std::u16string name;
  // Account names the Role is granted to, in browse order. EMPTY is a real
  // answer: a Role nobody holds.
  std::vector<std::u16string> members;
  // True for the eight well-known Roles (Part 3 §4.9). Their NODES cannot be
  // deleted, though their membership is editable like any other Role's.
  bool well_known = false;
  // What the SERVER says this Role grants — its entry in the published
  // RolePermissions map, never a client-side table.
  //
  // nullopt has exactly one meaning: the server published no entry for this
  // Role. That is the honest answer for a custom (group) Role, whose grants
  // are a per-namespace policy (Part 3 §5.2.9) this client cannot read.
  // "The map could not be read" is NOT represented here — that fails the whole
  // read (see `ReadRoleMemberships`), so an absent entry never has to stand in
  // for two different facts.
  std::optional<scada::Permission> permissions;
};

// Reads every Role, its membership, and what the server grants it.
//
// Returns nullopt when the RoleSet could not be browsed at all — a server
// always publishes the well-known Roles, so "no Roles" is not a state a
// working read produces, and rendering it as an empty list would state a fact
// the client does not have (docs/client/ux/principles.md §5).
//
// Also nullopt when the server's role -> permission map could not be read.
// That is deliberately all-or-nothing: this client MUST NOT carry its own copy
// of the map, so without it there is nothing truthful to say about what any
// Role means, and the surfaces already render an unresolved RoleSet correctly.
//
// Only UserName criteria are read: the other IdentityCriteriaType values
// describe token classes a username/password account never presents (Part 18
// §4.4.4), so treating them as account names would invent memberships.
Awaitable<std::optional<std::vector<RoleMembership>>> ReadRoleMemberships(
    AnyExecutor executor,
    NodeService& node_service,
    scada::AttributeService& attribute_service);

// Reads the server's role -> permission map: the RolePermissions attribute of
// the Server object (OPC UA Part 3 §5.2.9), which for a node carrying no
// per-node override publishes the server-wide defaults — every well-known Role
// paired with what it grants.
//
// This is the ONE place the client learns what a Role means. Reading it is
// what stops the client from having an opinion the server can contradict: a
// stale local table is exactly how a client comes to tell an operator that an
// account may do something the server will refuse.
//
// Returns nullopt when the attribute could not be read, including the
// Bad_UserAccessDenied an anonymous session gets (the map needs the
// ReadRolePermissions permission, which every authenticated Role holds).
Awaitable<std::optional<std::vector<scada::RolePermissionType>>>
ReadServerRolePermissions(scada::AttributeService& attribute_service);

// One Role an account holds. The id travels with the name because the two
// answer different questions: the name is what an operator reads, the id is
// what the effective permissions are derived from (a display name is
// server-supplied text and must never be the key of an authorization
// decision).
struct AccountRole {
  scada::NodeId node_id;
  std::u16string name;
  // The grant the server published for this Role, carried through from
  // `RoleMembership::permissions` so a permission breakdown never has to look
  // anything up in a client-side map. nullopt means the server published no
  // entry — a custom (group) Role, which therefore contributes nothing.
  std::optional<scada::Permission> permissions;
};

// Inverts `roles` into account name -> the Roles it holds, which is what a
// per-user surface needs. An account named by no rule is simply absent, and
// the caller decides whether that means "holds nothing" or "unknown".
std::map<std::u16string, std::vector<AccountRole>> RolesByAccount(
    const std::vector<RoleMembership>& roles);
