#include "views/client_utils_views.h"

#include "base/win/win_util2.h"
#include "model/scada_node_ids.h"
#include "common_resources.h"
#include "components/main/context_menu_model.h"
#include "components/write/write_dialog.h"
#include "controller.h"
#include "remote/session_proxy.h"
#include "window_info.h"

#include <atlbase.h>

#include <atlapp.h>
#include <atluser.h>

void ShowPopupMenu(gfx::NativeView native_view,
                   HMENU popup_menu,
                   const gfx::Point& point,
                   bool right_click) {
  UINT flags = right_click ? TPM_RIGHTBUTTON : 0;
  TrackPopupMenu(popup_menu, flags, point.x(), point.y(), 0, native_view, NULL);
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
      std::wstring label = model.GetLabelAt(i);

      if (type == ui::MenuModel::TYPE_SUBMENU) {
        ui::MenuModel* submenu_model = model.GetSubmenuModelAt(i);
        if (submenu_model) {
          WTL::CMenuHandle submenu;
          submenu.CreateMenu();
          BuildMenu(submenu, *submenu_model, 0);
          menu.InsertMenu(position++, MFT_STRING | MF_BYPOSITION, submenu,
                          label.c_str());
        }

      } else {
        unsigned command_id = model.GetCommandIdAt(i);
        menu.InsertMenu(position++, MFT_STRING | MF_BYPOSITION, command_id,
                        label.c_str());
      }
    }
  }
}

HMENU CreatePopupMenu(unsigned resource_id, ui::MenuModel& context_menu_model) {
  WTL::CMenuHandle menu;
  menu.LoadMenu(resource_id);

  WTL::CMenuHandle popup = menu.GetSubMenu(0);
  assert(popup);

  int pos = win_util::RemoveMenuCommand(popup, ID_ITEM_COMMANDS);
  if (pos != -1)
    BuildMenu(popup, context_menu_model, pos);
  win_util::RemoveConsequentMenuSeparators(popup);

  return menu;
}

std::wstring Translate(const char* text) {
  return base::SysNativeMBToWide(text);
}

std::wstring FormatHostName(const std::string& host_name) {
  if (host_name.empty()) {
    static const std::wstring local_server_string =
        win_util::LoadResourceString(WTL::ModuleHelper::GetResourceInstance(),
                                     IDS_LOCAL_SERVER);
    return local_server_string;
  } else {
    return base::SysNativeMBToWide(host_name);
  }
}
