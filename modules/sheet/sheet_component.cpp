#include "modules/sheet/sheet_component.h"

#include "aui/translation.h"
#include "controller/command_ui_registry.h"
#include "controller/controller_registry.h"
#include "modules/sheet/sheet_view.h"
#include "resources/common_resources.h"

// NOTE: Context menu depends on edit mode.
const WindowInfo kSheetWindowInfo = {
    ID_SHEET_VIEW, "CusTable", u"Custom Table", WIN_INS, 0, 0, IDR_ITEM_POPUP};

REGISTER_CONTROLLER(SheetController, kSheetWindowInfo);

void RegisterSheetCommandActions(UiCommandRegistry& ui_command_registry) {
  ui_command_registry.AddMenuItem({.menu_id = MainMenuId::Table,
                                   .order = 110,
                                   .command_id = ID_SHEET_VIEW,
                                   .title = Translate("New Custom Table")});
}
