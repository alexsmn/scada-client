#include "user_access/users_grid.h"

#include "model/security_node_ids.h"
#include "node_service/node_ref.h"
#include "node_service/node_util.h"
#include "scada/basic_types.h"
#include "scada/standard_node_ids.h"
#include "scada/variant.h"

#include <optional>
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

    // Both reads await their fetch, so a miss here is a genuine absence — the
    // property does not exist, the fetch failed, or the value arrived in an
    // unreadable type. Neither may be folded into its "default": zero is a
    // valid AccessRights (Observer, view only) and false a valid MultiSessions
    // (single session), so get_or() would turn "we could not read this" into a
    // confident, plausible, wrong row. Variant::get() reports the difference;
    // the row then carries kUnknown / nullopt through to the cells. See
    // docs/ux/principles.md §5.
    UserRole role = UserRole::kUnknown;
    if (NodeRef rights = child[scada::security::id::UserType_AccessRights]) {
      co_await rights.Fetch(NodeFetchStatus::NodeOnly);
      scada::Int32 access = 0;
      if (rights.value().get(access))
        role = UserRoleFor(access);
    }

    std::optional<bool> multi;
    if (NodeRef sessions = child[scada::security::id::UserType_MultiSessions]) {
      co_await sessions.Fetch(NodeFetchStatus::NodeOnly);
      bool value = false;
      if (sessions.value().get(value))
        multi = value;
    }

    rows.push_back(UserGridRow{.node_id = child.node_id(),
                               .name = ToString16(child.display_name()),
                               .role = role,
                               .multi_sessions = multi});
  }
  co_return rows;
}
