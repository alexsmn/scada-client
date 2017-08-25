#pragma once

#include <algorithm>
#include <memory>
#include <vector>

using std::min;
using std::max;

#include <atlbase.h>
#include <wtl/atlapp.h>
#include <wtl/atlframe.h>

#include "base/strings/string16.h"
#include "common_resources.h"
#include "components/main/views/main_menu_model2.h"

class ActionManager;

class MainMenu {
 public:
  MainMenu(MainWindowViews& main_view, HMENU hmenu, ActionManager& action_manager, Profile& profile,
      FileCache& file_cache, Favourites& favourites);

  BEGIN_MSG_MAP(MainMenu)
    MESSAGE_HANDLER(WM_INITMENUPOPUP, OnInitMenuPopup)
    MESSAGE_HANDLER(WM_MENUCOMMAND, OnMenuCommand)
    MESSAGE_HANDLER(WM_MENUSELECT, OnMenuSelect)
    MESSAGE_HANDLER(WM_COMMAND, OnCommand)
  END_MSG_MAP()

 private:
  void CreateMenuItems(HMENU hmenu, int& pos, UINT& id, const MainMenuModel2::MenuItems& items);

  LRESULT OnInitMenuPopup(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT OnMenuCommand(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT OnMenuSelect(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT OnCommand(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

  ActionManager& action_manager_;

  std::unique_ptr<MainMenuModel2> model_;

  HMENU hmenu_;

  int context_menu_index_;
};
