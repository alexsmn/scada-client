#include "components/main/views/main_menu.h"

#include "base/color.h"
#include "command_handler.h"
#include "components/main/views/context_menu_model.h"
#include "views/client_utils_views.h"

MainMenu::MainMenu(MainWindowViews& main_window,
                   ActionManager& action_manager,
                   Favourites& favourites,
                   FileCache& file_cache,
                   Profile& profile,
                   DialogService& dialog_service,
                   MainWindowManager& main_window_manager,
                   ViewManager& view_manager,
                   unsigned resource_id)
    : model_{std::make_unique<MainMenuModel2>(MainMenuModel2Context{
          main_window, favourites, file_cache, profile, dialog_service,
          main_window_manager, view_manager})},
      action_manager_{action_manager} {
  handle_.LoadMenu(resource_id);

  for (int i = 0; i < handle_.GetMenuItemCount(); ++i) {
    CMenuItemInfo mii;
    base::char16 buf[30] = L"";
    mii.fMask = MIIM_FTYPE | MIIM_STRING;
    mii.dwTypeData = buf;
    mii.cch = sizeof(buf);
    if (handle_.GetMenuItemInfo(i, TRUE, &mii)) {
      if (_wcsicmp(mii.dwTypeData, L"<Context>") == 0) {
        mii.dwTypeData = L"Объект";
        handle_.SetMenuItemInfo(i, TRUE, &mii);
        context_menu_index_ = i;
      }
    }
  }
}

void MainMenu::CreateMenuItems(HMENU hmenu,
                               int& pos,
                               UINT& id,
                               const MainMenuModel2::MenuItems& items) {
  WTL::CMenuHandle menu(hmenu);

  for (auto& info : items) {
    MENUITEMINFO item;
    memset(&item, 0, sizeof(item));
    item.cbSize = sizeof(item);
    item.fMask = MIIM_FTYPE | MIIM_ID | MIIM_STRING | MIIM_STATE;
    item.fType = MFT_STRING;
    if (info.radio)
      item.fType |= MFT_RADIOCHECK;
    item.wID = id++;
    item.dwTypeData = const_cast<LPTSTR>(info.text.c_str());
    if (info.checked)
      item.fState |= MFS_CHECKED;
    if (!info.enabled)
      item.fState |= MFS_GRAYED;

    menu.InsertMenuItem(pos++, TRUE, &item);
  }
}

LRESULT MainMenu::OnInitMenuPopup(UINT /*uMsg*/,
                                  WPARAM wParam,
                                  LPARAM lParam,
                                  BOOL& bHandled) {
  HMENU hmenu = reinterpret_cast<HMENU>(wParam);
  bool is_window_menu = HIWORD(lParam) != 0;
  int index = LOWORD(lParam);

  if (!::IsMenu(hmenu))
    return 0;

  WTL::CMenuHandle menu(hmenu);

  if (handle_.GetSubMenu(index) == menu && index == context_menu_index_) {
    for (int i = menu.GetMenuItemCount() - 1; i >= 0; --i)
      menu.DeleteMenu(i, MF_BYPOSITION);
    // if (main_window.active_view())
    //  InsertMenuItemCommands(menu, 0, *main_window.active_view());
    ContextMenuModel model{model_->main_window_, action_manager_};
    BuildMenu(menu, model, 0);
    return 0;
  }

  int pos = 0;
  MainMenuModel2::MenuItems items;

  while (pos < menu.GetMenuItemCount()) {
    UINT id = menu.GetMenuItemID(pos);

    if (model_->GetMenuItems(id, items)) {
      assert(!items.empty());
      UINT id2 = id;
      while (menu.DeleteMenu(id2, MF_BYCOMMAND))
        id2++;
      CreateMenuItems(menu, pos, id, items);
      items.clear();

    } else if (id == ID_COLORS) {
      menu.DeleteMenu(pos, MF_BYPOSITION);
      int n = 0;
      for (size_t i = 0; i < palette::GetColorCount(); i++) {
        MENUITEMINFO item;
        memset(&item, 0, sizeof(item));
        item.cbSize = sizeof(item);
        item.fMask = MIIM_FTYPE | MIIM_ID | MIIM_STRING | MIIM_STATE;
        item.fType = MFT_STRING | MFT_OWNERDRAW;
        item.wID = ID_COLOR_0 + n++;
        item.dwTypeData = (LPTSTR)(LPCTSTR)palette::GetColorName(i);
        menu.InsertMenuItem(pos++, TRUE, &item);
      }

    } else {
      if (id != 0 && id != static_cast<UINT>(-1)) {
        UINT state = 0;
        if (!model_->IsCommandIdEnabled(id))
          state |= MFS_DISABLED;
        if (model_->IsCommandIdChecked(id))
          state |= MFS_CHECKED;

        MENUITEMINFO mii = {sizeof(MENUITEMINFO)};
        mii.fMask = MIIM_STATE;
        mii.fState = state;
        menu.SetMenuItemInfo(pos, TRUE, &mii);
      }

      pos++;
    }
  }

  return 0;
}

LRESULT MainMenu::OnMenuCommand(UINT /*uMsg*/,
                                WPARAM wParam,
                                LPARAM lParam,
                                BOOL& /*bHandled*/) {
  int index = static_cast<int>(wParam);
  HMENU menu = reinterpret_cast<HMENU>(lParam);

  // main_window.main_menu_->OnMenuCommand(index, menu);

  return 0;
}

LRESULT MainMenu::OnCommand(UINT uMsg,
                            WPARAM wParam,
                            LPARAM lParam,
                            BOOL& bHandled) {
  LRESULT result = 0;
  bHandled = model_->ProcessWindowMessage(NULL, uMsg, wParam, lParam, result);
  return result;
}

LRESULT MainMenu::OnMenuSelect(UINT /*uMsg*/,
                               WPARAM wParam,
                               LPARAM lParam,
                               BOOL& /*bHandled*/) {
  int index = LOWORD(wParam);
  HMENU menu = reinterpret_cast<HMENU>(lParam);

  // main_window.main_menu_->OnMenuSelect(index, menu);

  return 0;
}
