#pragma once

#include "base/any_executor.h"
#include "base/awaitable.h"
#include "scada/node_id.h"

#include <map>
#include <optional>
#include <string>
#include <vector>

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
};

// Reads every Role and its membership.
//
// Returns nullopt when the RoleSet could not be browsed at all — a server
// always publishes the well-known Roles, so "no Roles" is not a state a
// working read produces, and rendering it as an empty list would state a fact
// the client does not have (docs/client/ux/principles.md §5).
//
// Only UserName criteria are read: the other IdentityCriteriaType values
// describe token classes a username/password account never presents (Part 18
// §4.4.4), so treating them as account names would invent memberships.
Awaitable<std::optional<std::vector<RoleMembership>>> ReadRoleMemberships(
    AnyExecutor executor,
    NodeService& node_service);

// One Role an account holds. The id travels with the name because the two
// answer different questions: the name is what an operator reads, the id is
// what the effective permissions are derived from (a display name is
// server-supplied text and must never be the key of an authorization
// decision).
struct AccountRole {
  scada::NodeId node_id;
  std::u16string name;
};

// Inverts `roles` into account name -> the Roles it holds, which is what a
// per-user surface needs. An account named by no rule is simply absent, and
// the caller decides whether that means "holds nothing" or "unknown".
std::map<std::u16string, std::vector<AccountRole>> RolesByAccount(
    const std::vector<RoleMembership>& roles);
