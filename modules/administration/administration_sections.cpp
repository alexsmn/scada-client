#include "administration/administration_sections.h"

#include "resources/common_resources.h"

namespace {

// Order follows the admin screen: the people surfaces first, then the
// configuration tables. Every section the screen draws is now built; a future
// one joins this list the same way, and until it resolves it is simply not
// offered.
constexpr AdministrationSection kSections[] = {
    {.command_id = ID_USERS_VIEW, .label = "Users"},
    // Role membership IS the authorization model, so it sits directly under
    // Users rather than among the configuration tables.
    {.command_id = ID_ROLES_VIEW, .label = "Roles"},
    {.command_id = ID_PASSWORD_POLICY_VIEW, .label = "Password policy"},
    {.command_id = ID_AUDIT_LOG_VIEW, .label = "Audit log"},
    {.command_id = ID_HISTORICAL_DB_VIEW, .label = "Databases"},
    {.command_id = ID_TS_FORMATS_VIEW, .label = "Formats"},
    {.command_id = ID_SIMULATION_ITEMS_VIEW, .label = "Simulated Signals"},
};

// Deliberately absent: ID_TABLE_EDITOR ("Configuration"). It is a registered,
// admin-gated view with no entry point anywhere in the shell — listing it here
// would be this change's side effect of newly exposing a surface nobody
// reaches today, which is a routing decision of its own, not part of building
// the Administration pane.

}  // namespace

std::span<const AdministrationSection> GetAdministrationSections() {
  return kSections;
}
