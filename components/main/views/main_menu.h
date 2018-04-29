#pragma once

#include "base/strings/string16.h"
#include "common_resources.h"
#include "components/main/views/main_menu_model2.h"

#include <algorithm>
#include <memory>
#include <vector>

using std::max;
using std::min;

#include <atlbase.h>

#include <atlapp.h>
#include <atlframe.h>

class ActionManager;
class CommandHandler;
class Favourites;
class FileCache;
class Profile;

struct MainMenuContext {
  MainWindowViews& main_window_;
  ActionManager& action_manager_;
  Favourites& favourites_;
  FileCache& file_cache_;
  Profile& profile_;
  DialogService& dialog_service_;
  MainWindowManager& main_window_manager_;
  ViewManager& view_manager_;
  unsigned resource_id_;
};

class MainMenu : private MainMenuContext {
 public:
  explicit MainMenu(MainMenuContext&& context);

  CMenuHandle handle() { return handle_; }

  BEGIN_MSG_MAP(MainMenu)
  MESSAGE_HANDLER(WM_INITMENUPOPUP, OnInitMenuPopup)
  MESSAGE_HANDLER(WM_MENUCOMMAND, OnMenuCommand)
  MESSAGE_HANDLER(WM_MENUSELECT, OnMenuSelect)
  MESSAGE_HANDLER(WM_COMMAND, OnCommand)
  END_MSG_MAP()

 private:
  void CreateMenuItems(HMENU hmenu,
                       int& pos,
                       UINT& id,
                       const MainMenuModel2::MenuItems& items);

  LRESULT OnInitMenuPopup(UINT /*uMsg*/,
                          WPARAM /*wParam*/,
                          LPARAM /*lParam*/,
                          BOOL& /*bHandled*/);
  LRESULT OnMenuCommand(UINT /*uMsg*/,
                        WPARAM /*wParam*/,
                        LPARAM /*lParam*/,
                        BOOL& /*bHandled*/);
  LRESULT OnMenuSelect(UINT /*uMsg*/,
                       WPARAM /*wParam*/,
                       LPARAM /*lParam*/,
                       BOOL& /*bHandled*/);
  LRESULT OnCommand(UINT /*uMsg*/,
                    WPARAM /*wParam*/,
                    LPARAM /*lParam*/,
                    BOOL& /*bHandled*/);

  std::unique_ptr<MainMenuModel2> model_;

  CMenuHandle handle_;

  int context_menu_index_ = -1;
};
