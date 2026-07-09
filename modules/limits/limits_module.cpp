#include "limits/limits_module.h"

#include "aui/translation.h"
#include "base/awaitable.h"
#include "controller/action.h"
#include "controller/command_registry.h"
#include "controller/command_ui_registry.h"
#include "controller/selection_model.h"
#include "core/selection_command_context.h"
#include "model/data_items_node_ids.h"
#include "modules/limits/limit_dialog.h"
#include "node_service/node_util.h"
#include "resources/common_resources.h"
#include "scada/session_service.h"

LimitsModule::LimitsModule(LimitsModuleContext&& context)
    : LimitsModuleContext{std::move(context)} {
  ui_command_registry_.AddAction(Action{.command_id_ = ID_EDIT_LIMITS,
                                        .category_ = CATEGORY_ITEM,
                                        .title_ = Translate("Limits..."),
                                        .short_title_ = Translate("Limits")});

  selection_commands_.AddCommand(
      {.command_id = ID_EDIT_LIMITS,
       .execute_handler =
           [this](const SelectionCommandContext& context) {
             // `ShowLimitsDialog` returns a lazy awaitable — spawn it detached
             // so the dialog actually opens.
             CoSpawn(executor_, [this, &dialog_service = context.dialog_service,
                                 node = context.selection.node()]() {
               return ShowLimitsDialog(dialog_service, {node, task_manager_});
             });
           },
       .available_handler =
           [this](const SelectionCommandContext& context) {
             return session_service_.HasPrivilege(scada::Privilege::Control) &&
                    IsInstanceOf(context.selection.node(),
                                 data_items::id::AnalogItemType);
           }});
}
