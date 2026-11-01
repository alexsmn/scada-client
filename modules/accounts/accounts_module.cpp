#include "accounts/accounts_module.h"

#include "accounts/account_roles_command_builder.h"
#include "accounts/account_status_command_builder.h"
#include "controller/command_registry.h"
#include "controller/command_ui_registry.h"

#include <utility>

AccountsModule::AccountsModule(AccountsModuleContext&& context)
    : AccountsModuleContext{std::move(context)} {
  RegisterAccountRolesCommandActions(ui_command_registry_);
  selection_commands_.AddCommand(
      AccountRolesCommandBuilder{.executor_ = executor_,
                                 .local_events_ = local_events_,
                                 .profile_ = profile_,
                                 .session_service_ = session_service_,
                                 .node_service_ = node_service_,
                                 .attribute_service_ = attribute_service_}
          .Build());

  RegisterAccountStatusCommandActions(ui_command_registry_);
  for (const bool disable : {false, true}) {
    selection_commands_.AddCommand(
        AccountStatusCommandBuilder{.executor_ = executor_,
                                    .local_events_ = local_events_,
                                    .profile_ = profile_,
                                    .session_service_ = session_service_,
                                    .node_service_ = node_service_}
            .Build(disable));
  }
}
