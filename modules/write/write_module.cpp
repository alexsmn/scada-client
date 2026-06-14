#include "write/write_module.h"

#include "aui/translation.h"
#include "controller/action.h"
#include "controller/command_registry.h"
#include "controller/command_ui_registry.h"
#include "controller/selection_model.h"
#include "core/selection_command_context.h"
#include "model/data_items_node_ids.h"
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
             (void)ExecuteWriteDialog(
                 context.dialog_service,
                 WriteContext{executor_, timed_data_service_,
                              context.selection.node().node_id(), profile_,
                              false});
           },
       .enabled_handler =
           [](const SelectionCommandContext& context) {
             // TODO: Use `scada::AttributeId::UserWriteMask` when available.
             // Allow writing to all variables. Except for data items: check
             // an output channel is present.
             auto node = context.selection.node();
             return !IsInstanceOf(node, data_items::id::DataItemType) ||
                    !node[data_items::id::DataItemType_Output]
                         .value()
                         .is_null();
           },
       .available_handler =
           [this](const SelectionCommandContext& context) {
             return session_service_.HasPrivilege(scada::Privilege::Control) &&
                    context.selection.node().node_class() ==
                        scada::NodeClass::Variable;
           }});

  selection_commands_.AddCommand(
      {.command_id = ID_WRITE_MANUAL,
       .execute_handler =
           [this](const SelectionCommandContext& context) {
             (void)ExecuteWriteDialog(
                 context.dialog_service,
                 WriteContext{executor_, timed_data_service_,
                              context.selection.node().node_id(), profile_,
                              true});
           },
       .available_handler =
           [this](const SelectionCommandContext& context) {
             return session_service_.HasPrivilege(scada::Privilege::Control) &&
                    IsInstanceOf(context.selection.node(),
                                 data_items::id::DataItemType);
           }});
}
