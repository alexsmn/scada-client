#include "components/main/opened_view.h"

#include "components/main/action_manager.h"
#include "components/main/main_window.h"
#include "controller.h"
#include "qt/client_utils_qt.h"
#include "simple_menu_command_handler.h"
#include "ui/base/models/simple_menu_model.h"
#include "window_definition.h"
#include "window_info.h"

#include <atlbase.h>

#include <atlapp.h>

#include <atluser.h>
#include <QMenu>
#include <QWidget>

namespace {

void BuildMenuModel(CMenuHandle menu_handle,
                    ui::MenuModel& context_menu_model,
                    ui::SimpleMenuModel& menu_model,
                    std::vector<std::unique_ptr<ui::MenuModel>>& submenus) {
  for (int i = 0; i < menu_handle.GetMenuItemCount(); ++i) {
    wchar_t title[64] = {};

    CMenuItemInfo menu_info;
    menu_info.fMask |= MIIM_FTYPE | MIIM_ID | MIIM_STRING | MIIM_SUBMENU;
    menu_info.cch = std::size(title);
    menu_info.dwTypeData = title;
    menu_handle.GetMenuItemInfo(i, TRUE, &menu_info);

    if (menu_info.hSubMenu) {
      auto submenu_model =
          std::make_unique<ui::SimpleMenuModel>(menu_model.delegate());
      BuildMenuModel(menu_info.hSubMenu, context_menu_model, *submenu_model,
                     submenus);
      menu_model.AddSubMenu(menu_info.wID, title, submenu_model.get());
      submenus.emplace_back(std::move(submenu_model));

    } else if (menu_info.fType & MFT_SEPARATOR) {
      menu_model.AddSeparator(ui::NORMAL_SEPARATOR);

    } else if (menu_info.wID == ID_ITEM_COMMANDS) {
      menu_model.AddInplaceMenu(&context_menu_model);

    } else {
      menu_model.AddItem(menu_info.wID, title);
    }
  }
}

}  // namespace

void OpenedView::Print() {}

void OpenedView::ShowPopupMenu(unsigned resource_id,
                               const UiPoint& point,
                               bool right_click) {
  if (resource_id == 0)
    resource_id = window_info().menu;

  if (resource_id == 0) {
    QMenu menu;
    BuildMenu(menu, context_menu_model_);
    menu.exec(point);
    return;
  }

  SimpleMenuCommandHandler command_handler{main_window_->commands()};
  ui::SimpleMenuModel menu_model{&command_handler};
  std::vector<std::unique_ptr<ui::MenuModel>> submenus;

  {
    CMenu resource_menu;
    resource_menu.LoadMenu(resource_id);
    BuildMenuModel(resource_menu.GetSubMenu(0), context_menu_model_, menu_model,
                   submenus);
  }

  QMenu menu;
  BuildMenu(menu, menu_model);
  menu.exec(point);
}
