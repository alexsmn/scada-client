#include "user_access/users_grid.h"

#include "model/security_node_ids.h"
#include "node_service/node_ref.h"
#include "node_service/node_util.h"
#include "scada/basic_types.h"
#include "scada/standard_node_ids.h"
#include "scada/variant.h"

#include <set>
#include <utility>

Awaitable<std::vector<UserGridRow>> BuildUsersGrid(AnyExecutor executor,
                                                   NodeRef users_folder) {
  std::vector<UserGridRow> rows;

  co_await users_folder.Fetch(NodeFetchStatus::NodeAndChildren);
  // A user can be reachable through more than one hierarchical reference
  // subtype; dedupe by NodeId so each appears once.
  std::set<scada::NodeId> seen;
  for (NodeRef& child :
       users_folder.targets(scada::id::HierarchicalReferences)) {
    co_await child.Fetch(NodeFetchStatus::NodeAndChildren);
    if (!IsInstanceOf(child, scada::security::id::UserType))
      continue;
    if (!seen.insert(child.node_id()).second)
      continue;

    // The AccessRights / MultiSessions declarations live on UserType; make its
    // children resident so operator[](declaration_id) resolves to the instance
    // property.
    co_await child.type_definition().Fetch(NodeFetchStatus::NodeAndChildren);

    scada::Int32 access = 0;
    if (NodeRef rights =
            child[scada::security::id::UserType_AccessRights]) {
      co_await rights.Fetch(NodeFetchStatus::NodeOnly);
      access = rights.value().get_or<scada::Int32>(0);
    }

    bool multi = false;
    if (NodeRef sessions =
            child[scada::security::id::UserType_MultiSessions]) {
      co_await sessions.Fetch(NodeFetchStatus::NodeOnly);
      multi = sessions.value().get_or<bool>(false);
    }

    rows.push_back(UserGridRow{.node_id = child.node_id(),
                               .name = std::u16string(child.display_name()),
                               .role = UserRoleFor(access),
                               .multi_sessions = multi});
  }
  co_return rows;
}
