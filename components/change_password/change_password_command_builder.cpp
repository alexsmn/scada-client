#include "change_password_command_builder.h"

#include "resources/common_resources.h"
#include "components/change_password/change_password_dialog.h"
#include "controller/selection_model.h"
#include "model/security_node_ids.h"
#include "node_service/node_util.h"
#include "scada/session_service.h"

BasicCommand<SelectionCommandContext> ChangePasswordCommandBuilder::Build() {
  return {
      .command_id = ID_CHANGE_PASSWORD,
      .execute_handler =
          [&local_events = local_events_,
           &profile = profile_](const SelectionCommandContext& context) {
            ShowChangePasswordDialog(
                context.dialog_service,
                ChangePasswordContext{context.selection.node(), local_events,
                                      profile});
          },
      .available_handler =
          [&session_service =
               session_service_](const SelectionCommandContext& context) {
            return session_service.HasPrivilege(scada::Privilege::Configure) &&
                   IsInstanceOf(context.selection.node(),
                                security::id::UserType);
          }};
}