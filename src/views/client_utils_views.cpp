#include "client/views/client_utils_views.h"

#include "base/win/win_util.h"
#include "client/commands/write_dialog.h"
#include "client/common_resources.h"
#include "client/views/context_menu_model.h"
#include "client/controller.h"
#include "client/components/main/opened_view.h"
#include "client/components/main/views/main_window_views.h"
#include "client/window_info.h"
#include "common/scada_node_ids.h"

#include <atlbase.h>
#include <wtl/atlapp.h>
#include <wtl/atluser.h>

void ShowPopupMenu(MainWindow* main_window, HMENU popup_menu,
                   const gfx::Point& point, bool right_click) {
  DCHECK(main_window);
  UINT flags = right_click ? TPM_RIGHTBUTTON : 0;
  TrackPopupMenu(popup_menu, flags, point.x(), point.y(), 0,
                 static_cast<MainWindowViews*>(main_window)->GetWindowHandle(), NULL);
}

int ShowMessageBox(DialogService& dialog_service, const base::char16* message,
                   const base::char16* title, unsigned types) {
  return ::AtlMessageBox(static_cast<DialogServiceViews&>(dialog_service).GetParentView(), message, title, types);
}

void BuildMenu(HMENU hmenu, ui::MenuModel& model, int start_position) {
  model.MenuWillShow();

  WTL::CMenuHandle menu(hmenu);

  int position = start_position;
  int count = model.GetItemCount();
  for (int i = 0; i < count; ++i) {
    ui::MenuModel::ItemType type = model.GetTypeAt(i);
   
    if (type == ui::MenuModel::TYPE_SEPARATOR) {
      menu.InsertMenu(position++, MFT_SEPARATOR | MF_BYPOSITION);
    
    } else {
      base::string16 label = model.GetLabelAt(i);
      
      if (type == ui::MenuModel::TYPE_SUBMENU) {
        ui::MenuModel* submenu_model = model.GetSubmenuModelAt(i);
        if (submenu_model) {
          WTL::CMenuHandle submenu;
          submenu.CreateMenu();
          BuildMenu(submenu, *submenu_model, 0);
          menu.InsertMenu(position++, MFT_STRING | MF_BYPOSITION, submenu, label.c_str());
        }
    
      } else {
        unsigned command_id = model.GetCommandIdAt(i);
        menu.InsertMenu(position++, MFT_STRING | MF_BYPOSITION, command_id, label.c_str());
      }
    }
  }
}

HMENU CreatePopupMenu(unsigned resource_id, OpenedView& view, ActionManager& action_manager) {
  WTL::CMenuHandle menu;
  menu.LoadMenu(resource_id);

  WTL::CMenuHandle popup = menu.GetSubMenu(0);
  assert(popup);

  int pos = win_util::RemoveMenuCommand(popup, ID_ITEM_COMMANDS);
  if (pos != -1) {
    ContextMenuModel model(static_cast<MainWindowViews*>(view.main_window()), action_manager);
    BuildMenu(popup, model, pos);
  }
  win_util::RemoveConsequentMenuSeparators(popup);

  return menu;
}
