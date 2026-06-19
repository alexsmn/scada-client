#include "modules/table/table_component.h"

#include "aui/translation.h"
#include "base/awaitable.h"
#include "base/u16format.h"
#include "common/formula_util.h"
#include "controller/action.h"
#include "controller/command_registry.h"
#include "controller/command_ui_registry.h"
#include "controller/controller_registry.h"
#include "controller/selection_model.h"
#include "core/global_command_context.h"
#include "core/selection_command_context.h"
#include "main_window/main_menu/main_menu_model.h"
#include "main_window/main_window_interface.h"
#include "model/data_items_node_ids.h"
#include "model/scada_node_ids.h"
#include "modules/node_table/node_table_component.h"
#include "modules/selection_command_helpers.h"
#include "modules/table/table_view.h"
#include "node_service/node_util.h"
#include "profile/window_definition.h"
#include "resources/common_resources.h"
#include "ui/common/client_utils.h"

#include <utility>

namespace {

bool CanCreateSomething(const NodeRef& node) {
  if (node.target(scada::id::Creates)) {
    return true;
  }

  for (auto type = node.type_definition(); type; type = type.supertype()) {
    if (type.target(scada::id::Creates)) {
      return true;
    }
  }

  return false;
}

WindowDefinition MakeGroupWindowDefinition(const WindowInfo& window_info,
                                           const NodeRef& node,
                                           const NodeIdSet& node_ids) {
  WindowDefinition window_def{window_info};
  window_def.title =
      u16format(L"{}: {}", window_info.title, ToString16(node.display_name()));

  for (const auto& node_id : node_ids) {
    window_def.AddItem("Item").SetString("path", MakeNodeIdFormula(node_id));
  }

  return window_def;
}

Awaitable<void> OpenGroupTable(AnyExecutor executor,
                               MainWindowInterface& main_window,
                               NodeRef node) {
  auto parent = node.parent();
  if (!IsInstanceOf(parent, data_items::id::DataGroupType)) {
    co_return;
  }

  auto node_ids = co_await ExpandGroupItemIdsAsync(executor, parent);
  co_await main_window.OpenView(
      MakeGroupWindowDefinition(kTableWindowInfo, node, node_ids));
  co_return;
}

}  // namespace

const WindowInfo kTableWindowInfo = {ID_TABLE_VIEW,           "Table", u"Table",
                                     WIN_INS | WIN_CAN_PRINT, 620,     400};

REGISTER_CONTROLLER(TableView, kTableWindowInfo);

TableModule::TableModule(TableModuleContext&& context)
    : TableModuleContext{std::move(context)} {
  RegisterMainMenuFavouritesWindowType(MainMenuId::Table,
                                       kTableWindowInfo.name);
  RegisterTableCommandActions(ui_command_registry_);
  global_commands_.AddCommand(
      BasicCommand<GlobalCommandContext>{ID_OPEN_TABLE}.set_execute_handler(
          [executor = executor_](const GlobalCommandContext& context) {
            CoSpawn(executor,
                    [&main_window = context.main_window]() -> Awaitable<void> {
                      co_await main_window.OpenView(
                          WindowDefinition{kTableWindowInfo});
                    });
          }));
  selection_commands_.AddCommand(
      MakeOpenViewSelectionCommand(ID_OPEN_TABLE, kTableWindowInfo, executor_));
  selection_commands_.AddCommand(BasicCommand<SelectionCommandContext>{
      .command_id = ID_OPEN_GROUP_TABLE,
      .execute_handler =
          [executor = executor_](const SelectionCommandContext& context) {
            CoSpawn(executor, [executor, &main_window = context.main_window,
                               node = context.selection.node()]() mutable {
              return OpenGroupTable(executor, main_window, std::move(node));
            });
          },
      .available_handler =
          [](const SelectionCommandContext& context) {
            return context.selection.timed_data().connected();
          }});
  selection_commands_.AddCommand(MakeOpenSingleSelectionCommand(
      ID_TABLE_CONFIG, kTableEditorWindowInfo, executor_,
      [&session_service =
           session_service_](const SelectionCommandContext& context) {
        return session_service.HasPrivilege(scada::Privilege::Configure) &&
               CanCreateSomething(context.selection.node());
      }));
}

TableModule::~TableModule() {
  UnregisterMainMenuFavouritesWindowType(MainMenuId::Table,
                                         kTableWindowInfo.name);
}

void RegisterTableCommandActions(UiCommandRegistry& ui_command_registry) {
  ui_command_registry.AddAction(Action{.command_id_ = ID_OPEN_TABLE,
                                       .category_ = CATEGORY_OPEN,
                                       .title_ = Translate("Table"),
                                       .image_id_ = ID_TABLE_VIEW,
                                       .flags_ = Action::ALWAYS_VISIBLE});
  ui_command_registry.AddAction(Action{.command_id_ = ID_OPEN_GROUP_TABLE,
                                       .category_ = CATEGORY_OPEN,
                                       .title_ = Translate("Group Table"),
                                       .flags_ = Action::VISIBLE});
  ui_command_registry.AddAction(
      Action{.command_id_ = ID_TABLE_CONFIG,
             .category_ = CATEGORY_EDIT,
             .title_ = Translate("Element Properties"),
             .short_title_ = Translate("Elements")});

  ui_command_registry.AddMenuItem({.menu_id = MainMenuId::Table,
                                   .order = 100,
                                   .command_id = ID_TABLE_VIEW,
                                   .title = Translate("New Table")});
  ui_command_registry.AddMenuItem({.menu_id = MainMenuId::Table,
                                   .order = 200,
                                   .command_id = ID_OPEN_GROUP_TABLE,
                                   .title = Translate("Group Table"),
                                   .separator_before = true});
}
