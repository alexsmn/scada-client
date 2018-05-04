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

namespace ui {
class MenuModel;
}

struct MainMenuContext {
  unsigned resource_id_;
  const std::shared_ptr<MainMenuModel2> model_;
  const std::shared_ptr<ui::MenuModel> context_menu_model_;
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

  CMenuHandle handle_;

  int context_menu_index_ = -1;
};
