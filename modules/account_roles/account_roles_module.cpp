#include "account_roles/account_roles_module.h"

#include "account_roles/account_roles_command_builder.h"
#include "controller/command_registry.h"
#include "controller/command_ui_registry.h"

#include <utility>

AccountRolesModule::AccountRolesModule(AccountRolesModuleContext&& context)
    : AccountRolesModuleContext{std::move(context)} {
  RegisterAccountRolesCommandActions(ui_command_registry_);
  selection_commands_.AddCommand(
      AccountRolesCommandBuilder{.executor_ = executor_,
                                 .local_events_ = local_events_,
                                 .profile_ = profile_,
                                 .session_service_ = session_service_,
                                 .node_service_ = node_service_,
                                 .attribute_service_ = attribute_service_}
          .Build());
}
