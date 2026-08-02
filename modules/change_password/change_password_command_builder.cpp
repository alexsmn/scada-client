#include "change_password_command_builder.h"

#include "aui/translation.h"
#include "controller/action.h"
#include "controller/command_ui_registry.h"
#include "controller/selection_model.h"
#include "model/security_node_ids.h"
#include "modules/change_password/change_password_dialog.h"
#include "node_service/node_util.h"
#include "resources/common_resources.h"
#include "scada/session_service.h"

void RegisterChangePasswordCommandActions(
    UiCommandRegistry& ui_command_registry) {
  ui_command_registry.AddAction(Action{.command_id_ = ID_CHANGE_PASSWORD,
                                       .category_ = CATEGORY_SPECIFIC,
                                       .title_ = Translate("Set Password..."),
                                       .short_title_ = Translate("Password")});
}

BasicCommand<SelectionCommandContext> ChangePasswordCommandBuilder::Build() {
  return {.command_id = ID_CHANGE_PASSWORD,
          .execute_handler =
              [executor = executor_, &local_events = local_events_,
               &profile = profile_, &session_service = session_service_,
               &node_service = node_service_](
                  const SelectionCommandContext& context) {
                // Self vs. administrator reset selects the standard method
                // (Part 18 §5.2.8 vs §5.2.7) and the form.
                const bool self_service =
                    context.selection.node().node_id() ==
                    session_service.GetUserId();
                ShowChangePasswordDialog(
                    context.dialog_service,
                    ChangePasswordContext{
                        .user_ = context.selection.node(),
                        .executor_ = executor,
                        .local_events_ = local_events,
                        .profile_ = profile,
                        .node_service_ = &node_service,
                        .self_service_ = self_service});
              },
          .available_handler =
              [&session_service =
                   session_service_](const SelectionCommandContext& context) {
                return session_service.HasAccessRight(
                           scada::AccessRight::kConfigure) &&
                       IsInstanceOf(context.selection.node(),
                                    scada::security::id::UserType);
              }};
}
