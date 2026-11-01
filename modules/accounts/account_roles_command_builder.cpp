#include "accounts/account_roles_command_builder.h"

#include "accounts/account_roles_dialog.h"
#include "accounts/account_status.h"
#include "aui/dialog_service.h"
#include "aui/translation.h"
#include "base/awaitable.h"
#include "base/u16format.h"
#include "controller/action.h"
#include "controller/command_ui_registry.h"
#include "controller/selection_model.h"
#include "events/local_event_util.h"
#include "model/security_node_ids.h"
#include "node_service/node_util.h"
#include "resources/common_resources.h"
#include "scada/session_service.h"
#include "user_access/role_membership.h"

namespace {

// Reads the RoleSet, then shows the dialog. The read is the precondition for
// an honest dialog: a checkbox state is a claim about membership, so with no
// RoleSet there is nothing truthful to draw and the failure is reported
// instead.
Awaitable<void> ShowAccountRolesAsync(
    AnyExecutor executor,
    DialogService& dialog_service,
    std::u16string account,
    LocalEvents& local_events,
    Profile& profile,
    NodeService& node_service,
    scada::AttributeService& attribute_service) {
  const std::u16string title =
      u16format(Translate("Changing roles of user {}"), account);
  auto roles =
      co_await ReadRoleMemberships(executor, node_service, attribute_service);
  if (!roles) {
    ReportRequestResult(title, scada::StatusCode::Bad_NotReadable, local_events,
                        profile);
    co_return;
  }

  ShowAccountRolesDialog(
      dialog_service, account, MakeAccountRoleChoices(*roles, account),
      [executor, account, roles = *roles, title, &local_events, &profile,
       &node_service](std::vector<scada::NodeId> desired) {
        auto changes = ComputeRoleMembershipChanges(roles, account, desired);
        if (changes.empty()) {
          return;
        }
        CoSpawn(
            executor,
            [account, changes = std::move(changes), title, &local_events,
             &profile, &node_service]() mutable -> Awaitable<void> {
              const scada::Status status = co_await ApplyRoleMembershipChanges(
                  node_service, account, std::move(changes));
              ReportAccountChangeResult(title, status, local_events, profile);
            });
      });
}

}  // namespace

void RegisterAccountRolesCommandActions(
    UiCommandRegistry& ui_command_registry) {
  ui_command_registry.AddAction(Action{.command_id_ = ID_ACCOUNT_ROLES,
                                       .category_ = CATEGORY_SPECIFIC,
                                       .title_ = Translate("Roles..."),
                                       .short_title_ = Translate("Roles")});
}

BasicCommand<SelectionCommandContext> AccountRolesCommandBuilder::Build() {
  return {.command_id = ID_ACCOUNT_ROLES,
          .execute_handler =
              [executor = executor_, &local_events = local_events_,
               &profile = profile_, &node_service = node_service_,
               &attribute_service =
                   attribute_service_](const SelectionCommandContext& context) {
                // The rule names the account by its name, which is what a
                // login is matched against — the same name ModifyUser takes.
                std::u16string account =
                    ToString16(context.selection.node().display_name());
                CoSpawn(executor,
                        [executor, &dialog_service = context.dialog_service,
                         account = std::move(account), &local_events, &profile,
                         &node_service,
                         &attribute_service]() mutable -> Awaitable<void> {
                          co_await ShowAccountRolesAsync(
                              executor, dialog_service, std::move(account),
                              local_events, profile, node_service,
                              attribute_service);
                        });
              },
          // Changing membership is a security-administration write the server
          // restricts to the Configure right (SecurityAdmin, Part 3 §4.9);
          // offering it to anyone else would only produce a refusal.
          .available_handler =
              [&session_service =
                   session_service_](const SelectionCommandContext& context) {
                return session_service.HasAccessRight(
                           scada::AccessRight::kConfigure) &&
                       IsInstanceOf(context.selection.node(),
                                    scada::security::id::UserType);
              }};
}
