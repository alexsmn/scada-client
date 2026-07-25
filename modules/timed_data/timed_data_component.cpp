#include "modules/timed_data/timed_data_component.h"

#include "aui/translation.h"
#include "controller/action.h"
#include "controller/command_registry.h"
#include "controller/command_ui_registry.h"
#include "controller/controller_registry.h"
#include "controller/main_menu_window_type_registry.h"
#include "modules/selection_command_helpers.h"
#include "modules/timed_data/timed_data_controller.h"
#include "resources/common_resources.h"

#include <utility>

const WindowInfo kTimedDataWindowInfo = {
    ID_TIMED_DATA_VIEW, "TimeVal", u"Data",
    WIN_INS | WIN_DISALLOW_NEW | WIN_CAN_PRINT};

REGISTER_CONTROLLER(TimedDataController, kTimedDataWindowInfo);

TimedDataModule::TimedDataModule(TimedDataModuleContext&& context)
    : TimedDataModuleContext{std::move(context)} {
  RegisterMainMenuFavouritesWindowType(MainMenuId::Table,
                                       kTimedDataWindowInfo.name);
  RegisterTimedDataCommandActions(ui_command_registry_);
  selection_commands_.AddCommand(MakeOpenViewSelectionCommand(
      ID_TIMED_DATA_VIEW, kTimedDataWindowInfo, executor_));
}

TimedDataModule::~TimedDataModule() {
  UnregisterMainMenuFavouritesWindowType(MainMenuId::Table,
                                         kTimedDataWindowInfo.name);
}

void RegisterTimedDataCommandActions(UiCommandRegistry& ui_command_registry) {
  ui_command_registry.AddAction(Action{.command_id_ = ID_TIMED_DATA_VIEW,
                                       .category_ = CATEGORY_OPEN,
                                       .title_ = Translate("Data"),
                                       .image_id_ = IDB_TIMED_DATA,
                                       .flags_ = Action::ALWAYS_VISIBLE});

  ui_command_registry.AddMenuItem({.menu_id = MainMenuId::Table,
                                   .order = 120,
                                   .command_id = ID_TIMED_DATA_VIEW,
                                   .title = Translate("New Data Table")});
}
