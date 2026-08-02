#include "change_password/change_password_module.h"

#include "change_password/change_password_command_builder.h"
#include "controller/command_registry.h"
#include "controller/command_ui_registry.h"

#include <utility>

ChangePasswordModule::ChangePasswordModule(
    ChangePasswordModuleContext&& context)
    : ChangePasswordModuleContext{std::move(context)} {
  RegisterChangePasswordCommandActions(ui_command_registry_);
  selection_commands_.AddCommand(
      ChangePasswordCommandBuilder{.executor_ = executor_,
                                   .local_events_ = local_events_,
                                   .profile_ = profile_,
                                   .session_service_ = session_service_,
                                   .node_service_ = node_service_}
          .Build());
}
