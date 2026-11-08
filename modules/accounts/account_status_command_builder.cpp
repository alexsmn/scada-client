#include "accounts/account_status_command_builder.h"

#include "accounts/account_status.h"
#include "aui/translation.h"
#include "base/awaitable.h"
#include "base/u16format.h"
#include "controller/action.h"
#include "controller/command_ui_registry.h"
#include "controller/selection_model.h"
#include "events/local_events.h"
#include "model/security_node_ids.h"
#include "node_service/node_awaitable.h"
#include "node_service/node_util.h"
#include "resources/common_resources.h"
#include "scada/session_service.h"
#include "services/server_compatibility.h"
#include "user_access/account_configuration.h"

namespace {

// Reads the account's current mask — fetching it first, since the command may
// run on a node that was only selected — and flips its Disabled bit.
Awaitable<void> SetAccountDisabledAsync(NodeRef user,
                                        bool disable,
                                        LocalEvents& local_events,
                                        Profile& profile,
                                        NodeService& node_service) {
  const std::u16string account = ToString16(user.display_name());
  const std::u16string title = u16format(
      disable ? Translate("Disabling user {}") : Translate("Enabling user {}"),
      account);
  co_await user.Fetch(NodeFetchStatus::NodeAndChildren);
  co_await FetchTypeChainStatus(user.type_definition());
  if (NodeRef property =
          user[scada::security::id::UserType_UserConfiguration]) {
    co_await property.Fetch(NodeFetchStatus::NodeOnly);
  }
  const auto current = ResidentUserConfiguration(user);
  if (!current) {
    // The status is the Part 18 UserConfiguration property, which a Server
    // older than 2.6 does not have: an account there cannot be disabled at
    // all, so say that rather than "Bad_NotReadable".
    local_events.ReportEvent(
        LocalEvents::SEV_ERROR,
        u16format(L"{} - {}", title,
                  Translate("The account status cannot be read from the "
                            "Server. A Server older than version 2.6 cannot "
                            "disable accounts.")));
    co_return;
  }
  const scada::Status status =
      co_await SetAccountDisabled(node_service, account, *current, disable);
  ReportAccountChangeResult(title, status, local_events, profile);
}

}  // namespace

void RegisterAccountStatusCommandActions(
    UiCommandRegistry& ui_command_registry) {
  ui_command_registry.AddAction(Action{.command_id_ = ID_ACCOUNT_ENABLE,
                                       .category_ = CATEGORY_SPECIFIC,
                                       .title_ = Translate("Enable Account"),
                                       .short_title_ = Translate("Enable")});
  ui_command_registry.AddAction(Action{.command_id_ = ID_ACCOUNT_DISABLE,
                                       .category_ = CATEGORY_SPECIFIC,
                                       .title_ = Translate("Disable Account"),
                                       .short_title_ = Translate("Disable")});
}

BasicCommand<SelectionCommandContext> AccountStatusCommandBuilder::Build(
    bool disable) {
  return {.command_id = static_cast<unsigned>(disable ? ID_ACCOUNT_DISABLE
                                                      : ID_ACCOUNT_ENABLE),
          .execute_handler =
              [executor = executor_, disable, &local_events = local_events_,
               &profile = profile_, &node_service = node_service_](
                  const SelectionCommandContext& context) {
                CoSpawn(
                    executor,
                    [user = context.selection.node(), disable, &local_events,
                     &profile, &node_service]() mutable -> Awaitable<void> {
                      co_await SetAccountDisabledAsync(std::move(user), disable,
                                                       local_events, profile,
                                                       node_service);
                    });
              },
          // Changing whether an account may sign in is an administrator's
          // write (Part 18 §5.2.7 ModifyUser); offering it to anyone else
          // would only produce a refusal.
          .available_handler =
              [&session_service = session_service_,
               disable](const SelectionCommandContext& context) {
                const NodeRef& user = context.selection.node();
                if (!session_service.HasAccessRight(
                        scada::AccessRight::kConfigure) ||
                    !IsInstanceOf(user, scada::security::id::UserType)) {
                  return false;
                }
                const auto mask = ResidentUserConfiguration(user);
                if (!mask) {
                  return true;
                }
                const bool disabled = scada::HasUserConfiguration(
                    *mask, scada::UserConfiguration::kDisabled);
                return disabled != disable;
              }};
}
