#include "write/write_module.h"

#include "aui/translation.h"
#include "base/awaitable.h"
#include "controller/action.h"
#include "controller/command_registry.h"
#include "controller/command_ui_registry.h"
#include "controller/selection_model.h"
#include "core/selection_command_context.h"
#include "model/data_items_node_ids.h"
#include "modules/write/write_availability.h"
#include "modules/write/write_dialog.h"
#include "node_service/node_util.h"
#include "resources/common_resources.h"
#include "scada/session_service.h"

WriteModule::WriteModule(WriteModuleContext&& context)
    : WriteModuleContext{std::move(context)} {
  ui_command_registry_.AddAction(Action{.command_id_ = ID_WRITE,
                                        .category_ = CATEGORY_ITEM,
                                        .title_ = Translate("Control..."),
                                        .short_title_ = Translate("Control"),
                                        .image_id_ = IDB_WRITE});
  ui_command_registry_.AddAction(
      Action{.command_id_ = ID_WRITE_MANUAL,
             .category_ = CATEGORY_ITEM,
             .title_ = Translate("Manual Input..."),
             .short_title_ = Translate("Manual Input"),
             .image_id_ = IDB_WRITE_MANUAL});

  selection_commands_.AddCommand(
      {.command_id = ID_WRITE,
       .execute_handler =
           [this](const SelectionCommandContext& context) {
             // `ExecuteWriteDialog` returns a lazy awaitable — spawn it
             // detached so the dialog actually opens.
             CoSpawn(executor_, [this, &dialog_service = context.dialog_service,
                                 node_id =
                                     context.selection.node().node_id()]() {
               return ExecuteWriteDialog(
                   dialog_service, WriteContext{executor_, timed_data_service_,
                                                node_id, profile_, false});
             });
           },
       // Both gates read the shared node rule (`GetWriteBlock`), which the
       // Inspector also uses to tell the operator why control is unavailable.
       // Split as before: a node that cannot be controlled at all hides the
       // command, a missing output channel only disables it.
       .enabled_handler =
           [](const SelectionCommandContext& context) {
             return GetWriteBlock(context.selection.node()) !=
                    WriteBlock::kNoOutputChannel;
           },
       .available_handler =
           [this](const SelectionCommandContext& context) {
             return session_service_.HasPrivilege(scada::Privilege::Control) &&
                    GetWriteBlock(context.selection.node()) !=
                        WriteBlock::kNotCommandable;
           },
       // The greyed entry names the same rule the gate applied.
       .disabled_reason_handler =
           [](const SelectionCommandContext& context) {
             return Translate(
                 WriteBlockText(GetWriteBlock(context.selection.node())));
           }});

  selection_commands_.AddCommand(
      {.command_id = ID_WRITE_MANUAL,
       .execute_handler =
           [this](const SelectionCommandContext& context) {
             // `ExecuteWriteDialog` returns a lazy awaitable — spawn it
             // detached so the dialog actually opens.
             CoSpawn(executor_, [this, &dialog_service = context.dialog_service,
                                 node_id =
                                     context.selection.node().node_id()]() {
               return ExecuteWriteDialog(
                   dialog_service, WriteContext{executor_, timed_data_service_,
                                                node_id, profile_, true});
             });
           },
       .available_handler =
           [this](const SelectionCommandContext& context) {
             return session_service_.HasPrivilege(scada::Privilege::Control) &&
                    IsInstanceOf(context.selection.node(),
                                 scada::data_items::id::DataItemType);
           }});
}
