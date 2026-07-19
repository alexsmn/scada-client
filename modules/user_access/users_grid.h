#pragma once

#include "base/any_executor.h"
#include "base/awaitable.h"
#include "scada/node_id.h"
#include "user_access/user_access.h"

#include <string>
#include <vector>

class NodeRef;

// One row of the reshell users-admin grid (the main region of users-admin.html):
// the identity, its coarse role, and its session policy — the fields the client
// node model actually carries for a UserType instance (display name +
// AccessRights + MultiSessions). Backend / Areas / Status / Last-login from the
// mockup are server-side session concepts absent from the client model.
struct UserGridRow {
  scada::NodeId node_id;
  std::u16string name;
  UserRole role = UserRole::kObserver;
  bool multi_sessions = false;
};

// Browses the Users folder's user instances and builds the grid rows, reading
// each user's AccessRights (→ role) and MultiSessions. Fetches what it reads, so
// it is safe on a freshly-opened folder. Returns an empty list for a folder with
// no users.
Awaitable<std::vector<UserGridRow>> BuildUsersGrid(AnyExecutor executor,
                                                   NodeRef users_folder);
