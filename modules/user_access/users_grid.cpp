#include "user_access/users_grid.h"

#include "user_access/role_membership.h"

#include "base/utf_convert.h"
#include "model/security_node_ids.h"
#include "node_service/node_ref.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "scada/basic_types.h"
#include "scada/standard_node_ids.h"
#include "scada/user_management_encoding.h"
#include "scada/variant.h"

#include <algorithm>
#include <map>
#include <utility>

namespace {

// Server.ServerConfiguration.UserManagement.Users — the standard account list
// (OPC UA Part 18 §5.2.2).
const scada::NodeId kUsersProperty{scada::id::UserManagement_Users,
                                   scada::NamespaceIndexes::NS0};
// Name -> UserType node id. The grid reads nothing else from the folder; this
// exists only so a selected row has something the shell can route to the
// inspector, and goes when the UserType folder does.
Awaitable<std::map<std::u16string, scada::NodeId>> ReadUserNodeIds(
    NodeService& node_service) {
  std::map<std::u16string, scada::NodeId> node_ids;
  NodeRef users_folder = node_service.GetNode(scada::security::id::Users);
  if (!users_folder) {
    co_return node_ids;
  }
  co_await users_folder.Fetch(NodeFetchStatus::NodeAndChildren);
  for (NodeRef& child :
       users_folder.targets(scada::id::HierarchicalReferences)) {
    co_await child.Fetch(NodeFetchStatus::NodeOnly);
    if (!IsInstanceOf(child, scada::security::id::UserType)) {
      continue;
    }
    node_ids.try_emplace(ToString16(child.display_name()), child.node_id());
  }
  co_return node_ids;
}

}  // namespace

Awaitable<std::optional<std::vector<UserGridRow>>> BuildUsersGrid(
    AnyExecutor executor,
    NodeService& node_service,
    scada::AttributeService& attribute_service) {
  NodeRef users_property = node_service.GetNode(kUsersProperty);
  if (!users_property) {
    co_return std::nullopt;
  }
  co_await users_property.Fetch(NodeFetchStatus::NodeOnly);

  // One Read for the whole account list. A bad read — including the
  // Bad_UserAccessDenied a non-administrator gets (Part 18 §5.2.1) — decodes
  // to nullopt, which the panel renders as "cannot be read" rather than as an
  // empty server.
  auto users = scada::DecodeUserManagementUsers(users_property.value());
  if (!users) {
    co_return std::nullopt;
  }

  // Role membership is the shared authorization source; the Roles view reads
  // the same list (role_membership.h).
  auto roles_read =
      co_await ReadRoleMemberships(executor, node_service, attribute_service);
  std::optional<std::map<std::u16string, std::vector<AccountRole>>>
      memberships;
  if (roles_read) {
    memberships = RolesByAccount(*roles_read);
  }

  auto node_ids = co_await ReadUserNodeIds(node_service);

  std::vector<UserGridRow> rows;
  rows.reserve(users->size());
  for (const scada::UserManagementDataType& user : *users) {
    const std::u16string name = UtfConvert<char16_t>(user.user_name);

    std::optional<std::vector<AccountRole>> roles;
    if (memberships) {
      auto i = memberships->find(name);
      // Absent from the map is a real answer — the account holds no Role —
      // which is why this defaults to an EMPTY vector and not to nullopt.
      roles = i != memberships->end() ? i->second : std::vector<AccountRole>{};
    }

    auto node_id = node_ids.find(name);
    rows.push_back(
        UserGridRow{.name = name,
                    .description = UtfConvert<char16_t>(user.description),
                    .user_configuration = user.user_configuration,
                    .roles = std::move(roles),
                    .node_id = node_id != node_ids.end() ? node_id->second
                                                         : scada::NodeId{}});
  }
  co_return rows;
}

const char* UserStatusLabelKey(scada::UserConfiguration user_configuration) {
  return scada::HasUserConfiguration(user_configuration,
                                     scada::UserConfiguration::kDisabled)
             ? "Disabled"
             : "Enabled";
}
