#pragma once

#include "base/any_executor.h"
#include "base/awaitable.h"
#include "scada/node_id.h"
#include "user_access/user_access.h"

#include <optional>
#include <string>
#include <vector>

class NodeRef;

// One row of the reshell users-admin grid (the main region of
// users-admin.html): the identity, its coarse role, and its session policy —
// the fields the client node model actually carries for a UserType instance
// (display name + AccessRights + MultiSessions). Backend / Areas / Status /
// Last-login from the mockup are server-side session concepts absent from the
// client model.
struct UserGridRow {
  scada::NodeId node_id;
  std::u16string name;
  // kUnknown when AccessRights could not be read. It is the default so a row
  // built without a resolved bitmask cannot claim a real role: zero is a valid
  // bitmask (Observer, view only), so folding an absent read into it would
  // state a fact the client does not have.
  UserRole role = UserRole::kUnknown;
  // Unset when MultiSessions could not be read — the same rule as `role`, since
  // false is likewise a valid answer ("single session") rather than an absence.
  std::optional<bool> multi_sessions;
};

// Browses the Users folder's user instances and builds the grid rows, reading
// each user's AccessRights (→ role) and MultiSessions. Fetches what it reads,
// so it is safe on a freshly-opened folder. Returns an empty list for a folder
// with no users.
Awaitable<std::vector<UserGridRow>> BuildUsersGrid(AnyExecutor executor,
                                                   NodeRef users_folder);
