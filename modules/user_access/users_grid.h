#pragma once

#include "base/any_executor.h"
#include "base/awaitable.h"
#include "scada/authorization.h"
#include "scada/node_id.h"
#include "user_access/role_membership.h"

#include <optional>
#include <string>
#include <vector>

namespace scada {
class AttributeService;
}

class NodeService;

// One row of the users-admin grid (the main region of users-admin.html).
//
// Sourced from the OPC UA standard user model: a single Read of
// `Server.ServerConfiguration.UserManagement.Users` returns every account as a
// `UserManagementDataType` (Part 18 §5.2.4), so the grid needs no per-user
// Browse. Because the whole list arrives in one read, `name`, `description`
// and `user_configuration` are either all real or the read failed and there
// are no rows at all — there is no per-field "could not read" case to
// represent.
struct UserGridRow {
  // The account name, and its identity: the standard user model keys by name,
  // and it is what a login is matched against.
  std::u16string name;
  std::u16string description;
  // The Part 18 §5.2.3 mask. `kDisabled` is what the Status column renders.
  scada::UserConfiguration user_configuration =
      scada::UserConfiguration::kNone;

  // The Roles the account holds, from the RoleSet identity mapping rules — the
  // stored authorization model since the access-rights bitmask stopped being
  // authoritative. Each carries its id as well as its name, because the
  // inspector derives the effective permissions from the id.
  //
  // An EMPTY vector is a real answer: an account with no role, i.e. a plain
  // authenticated user who may only look. `nullopt` means the RoleSet could
  // not be read, which must not be shown as "holds nothing" — that is the same
  // rule the quality bands follow (docs/client/ux/principles.md §5).
  std::optional<std::vector<AccountRole>> roles;

  // The account's UserType node, while one still exists.
  //
  // TRANSITIONAL: the standard model has no per-user node and this grid reads
  // nothing from it. It is carried only because the shell routes inspector
  // selection through a NodeRef, so a row needs something selectable. It goes
  // when the UserType folder does. Null when no UserType row matches the name.
  scada::NodeId node_id;
};

// Reads every account from the UserManagement object and joins the Roles each
// one holds.
//
// Returns nullopt when the Users property could not be read at all — an empty
// grid and a broken one must not look alike. An admin-less session gets
// `Bad_UserAccessDenied` from the server (Part 18 §5.2.1), which lands here as
// nullopt.
Awaitable<std::optional<std::vector<UserGridRow>>> BuildUsersGrid(
    AnyExecutor executor,
    NodeService& node_service,
    scada::AttributeService& attribute_service);

// The label for an account's Status cell, as an English literal for
// Translate(): a disabled account may not authenticate.
const char* UserStatusLabelKey(scada::UserConfiguration user_configuration);
