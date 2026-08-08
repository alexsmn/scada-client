#include "user_access/role_membership.h"

#include "base/utf_convert.h"
#include "model/security_node_ids.h"
#include "node_service/node_ref.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "scada/attribute_service.h"
#include "scada/authorization.h"
#include "scada/basic_types.h"
#include "scada/role_permission_encoding.h"
#include "scada/standard_node_ids.h"
#include "scada/variant.h"

#include <algorithm>

namespace {

const scada::NodeId kRoleSet{scada::id::Server_ServerCapabilities_RoleSet,
                             scada::NamespaceIndexes::NS0};

// The Server object. Its RolePermissions attribute carries no per-node
// override, so it publishes the server-wide default map (OPC UA Part 3
// §5.2.9) — which is precisely "what each Role grants on this server".
//
// There is deliberately no vendor property for this: OPC UA models grants
// per node (§5.2.9) and per namespace (Part 5 §6.3.13), never on RoleType,
// whose properties are Identities / Applications / Endpoints (Part 18 §4.4).
// So the standard way to ask "what does this Role grant" is to read the
// RolePermissions of a node, and the Server object is the natural anchor for
// the server-wide answer.
const scada::NodeId kServerObject{scada::id::Server,
                                  scada::NamespaceIndexes::NS0};

bool IsWellKnownRole(const scada::NodeId& node_id) {
  for (const scada::WellKnownRole role :
       {scada::WellKnownRole::kAnonymous,
        scada::WellKnownRole::kAuthenticatedUser,
        scada::WellKnownRole::kObserver, scada::WellKnownRole::kOperator,
        scada::WellKnownRole::kEngineer, scada::WellKnownRole::kSupervisor,
        scada::WellKnownRole::kConfigureAdmin,
        scada::WellKnownRole::kSecurityAdmin}) {
    if (scada::WellKnownRoleId(role) == node_id) {
      return true;
    }
  }
  return false;
}

// The published grant for `role_id`, or nullopt when the server published no
// entry for it.
std::optional<scada::Permission> PublishedGrant(
    std::span<const scada::RolePermissionType> published,
    const scada::NodeId& role_id) {
  for (const scada::RolePermissionType& entry : published) {
    if (entry.role_id == role_id) {
      return entry.permissions;
    }
  }
  return std::nullopt;
}

// The account name a rule grants to, or nullopt when the rule names something
// other than a user name (or could not be read).
Awaitable<std::optional<std::u16string>> ReadRuleCriteria(NodeRef rule) {
  co_await rule.Fetch(NodeFetchStatus::NodeAndChildren);
  if (!IsInstanceOf(rule, scada::security::id::IdentityMappingRuleType)) {
    co_return std::nullopt;
  }

  scada::Int32 criteria_type = 0;
  if (NodeRef type_node =
          rule[scada::security::id::IdentityMappingRuleType_CriteriaType]) {
    co_await type_node.Fetch(NodeFetchStatus::NodeOnly);
    if (!type_node.value().get(criteria_type)) {
      co_return std::nullopt;
    }
  }
  if (criteria_type !=
      static_cast<scada::Int32>(scada::IdentityCriteriaType::kUserName)) {
    co_return std::nullopt;
  }

  if (NodeRef criteria_node =
          rule[scada::security::id::IdentityMappingRuleType_Criteria]) {
    co_await criteria_node.Fetch(NodeFetchStatus::NodeOnly);
    scada::String criteria;
    if (criteria_node.value().get(criteria) && !criteria.empty()) {
      co_return UtfConvert<char16_t>(criteria);
    }
  }
  co_return std::nullopt;
}

}  // namespace

Awaitable<std::optional<std::vector<scada::RolePermissionType>>>
ReadServerRolePermissions(scada::AttributeService& attribute_service) {
  const scada::DataValue value = co_await scada::Read(
      attribute_service, scada::ServiceContext{},
      scada::ReadValueId{kServerObject,
                         scada::AttributeId::RolePermissions});
  if (!scada::IsGood(value.status_code)) {
    co_return std::nullopt;
  }
  co_return scada::DecodeRolePermissions(value.value);
}

Awaitable<std::optional<std::vector<RoleMembership>>> ReadRoleMemberships(
    AnyExecutor executor,
    NodeService& node_service,
    scada::AttributeService& attribute_service) {
  // What each Role grants comes from the server, not from this client. Without
  // it there is nothing truthful to say about any Role, so the whole read
  // reports unknown rather than falling back to an assumed map.
  const auto published = co_await ReadServerRolePermissions(attribute_service);
  if (!published) {
    co_return std::nullopt;
  }

  NodeRef role_set = node_service.GetNode(kRoleSet);
  if (!role_set) {
    co_return std::nullopt;
  }
  co_await role_set.Fetch(NodeFetchStatus::NodeAndChildren);

  std::vector<RoleMembership> roles;
  for (NodeRef& role : role_set.targets(scada::id::HierarchicalReferences)) {
    co_await role.Fetch(NodeFetchStatus::NodeAndChildren);

    RoleMembership membership{
        .node_id = role.node_id(),
        .name = ToString16(role.display_name()),
        .well_known = IsWellKnownRole(role.node_id()),
        .permissions = PublishedGrant(*published, role.node_id())};
    for (NodeRef& rule : role.targets(scada::id::HierarchicalReferences)) {
      if (auto member = co_await ReadRuleCriteria(rule)) {
        if (std::ranges::find(membership.members, *member) ==
            membership.members.end()) {
          membership.members.push_back(*std::move(member));
        }
      }
    }
    roles.push_back(std::move(membership));
  }

  // A RoleSet that browsed to nothing is indistinguishable from one that could
  // not be browsed, and a conformant server always publishes the well-known
  // Roles — so report unknown rather than "this server has no Roles".
  if (roles.empty()) {
    co_return std::nullopt;
  }
  co_return roles;
}

std::map<std::u16string, std::vector<AccountRole>> RolesByAccount(
    const std::vector<RoleMembership>& roles) {
  std::map<std::u16string, std::vector<AccountRole>> by_account;
  for (const RoleMembership& role : roles) {
    for (const std::u16string& member : role.members) {
      std::vector<AccountRole>& held = by_account[member];
      if (std::ranges::find(held, role.node_id, &AccountRole::node_id) ==
          held.end()) {
        held.push_back(AccountRole{role.node_id, role.name, role.permissions});
      }
    }
  }
  return by_account;
}
