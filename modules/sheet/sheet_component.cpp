#include "modules/sheet/sheet_component.h"

#include "aui/translation.h"
#include "controller/action.h"
#include "controller/command_ui_registry.h"
#include "controller/controller_registry.h"
#include "main_window/main_menu/main_menu_model.h"
#include "modules/sheet/sheet_view.h"
#include "resources/common_resources.h"

#include <utility>

// NOTE: Context menu depends on edit mode.
const WindowInfo kSheetWindowInfo = {
    ID_SHEET_VIEW, "CusTable", u"Custom Table", WIN_INS, 0, 0, IDR_ITEM_POPUP};

REGISTER_CONTROLLER(SheetController, kSheetWindowInfo);

SheetModule::SheetModule(SheetModuleContext&& context)
    : SheetModuleContext{std::move(context)} {
  RegisterMainMenuFavouritesWindowType(MainMenuId::Table,
                                       kSheetWindowInfo.name);
  RegisterSheetCommandActions(ui_command_registry_);
}

SheetModule::~SheetModule() {
  UnregisterMainMenuFavouritesWindowType(MainMenuId::Table,
                                         kSheetWindowInfo.name);
}

void RegisterSheetCommandActions(UiCommandRegistry& ui_command_registry) {
  ui_command_registry.AddAction(Action{.command_id_ = ID_EDIT,
                                       .category_ = CATEGORY_SETUP,
                                       .title_ = Translate("Edit"),
                                       .flags_ = Action::CHECKABLE});
  if (!ui_command_registry.action_manager().FindAction(ID_GRAPH_COLOR)) {
    ui_command_registry.AddAction(Action{.command_id_ = ID_GRAPH_COLOR,
                                         .category_ = CATEGORY_SETUP,
                                         .title_ = Translate("Line Color..."),
                                         .short_title_ = Translate("Color")});
  }

  ui_command_registry.AddMenuItem({.menu_id = MainMenuId::Table,
                                   .order = 110,
                                   .command_id = ID_SHEET_VIEW,
                                   .title = Translate("New Custom Table")});
}
