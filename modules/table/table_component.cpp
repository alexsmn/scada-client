#include "modules/table/table_component.h"

#include "aui/translation.h"
#include "controller/action.h"
#include "controller/command_ui_registry.h"
#include "controller/controller_registry.h"
#include "modules/table/table_view.h"
#include "resources/common_resources.h"

#include <utility>

const WindowInfo kTableWindowInfo = {ID_TABLE_VIEW,           "Table", u"Table",
                                     WIN_INS | WIN_CAN_PRINT, 620,     400};

REGISTER_CONTROLLER(TableView, kTableWindowInfo);

TableModule::TableModule(TableModuleContext&& context)
    : TableModuleContext{std::move(context)} {
  RegisterTableCommandActions(ui_command_registry_);
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
