#pragma once

#include <atlbase.h>
#include <atlwin.h>
#include <memory>

#include "common_resources.h"
#include "ui/base/models/simple_menu_model.h"

class DialogService;
class Favourites;
class FileCache;
class MainWindowViews;
class MainWindowManager;
class Profile;
class ViewManager;

struct MainMenuModel2Context {
  MainWindowViews& main_window_;
  Favourites& favourites_;
  FileCache& file_cache_;
  Profile& profile_;
  DialogService& dialog_service_;
  MainWindowManager& main_window_manager_;
  ViewManager& view_manager_;
};

class MainMenuModel2 : private MainMenuModel2Context,
                       public ui::SimpleMenuModel,
                       private ui::SimpleMenuModel::Delegate {
 public:
  explicit MainMenuModel2(MainMenuModel2Context&& context);

  struct MenuItem;
  typedef std::vector<MenuItem> MenuItems;

  struct MenuItem {
    MenuItem() : radio(false), checked(false), enabled(true) {}

    base::string16 text;
    bool radio;
    bool checked;
    bool enabled;

    MenuItems subitems;
  };

  bool GetMenuItems(UINT id, MenuItems& items);

 private:
  void GetFavouritesMenuItems(MenuItems& items,
                              int type_count,
                              const unsigned* types);

  void GetFileCacheItems(int type_id, MenuItems& items);

  BEGIN_MSG_MAP(MainMenuModel2)
  COMMAND_RANGE_HANDLER(ID_NEW_DISPLAY_0, ID_NEW_DISPLAY_0 + 99, OnNewDisplay)
  COMMAND_RANGE_HANDLER(ID_NEW_REPORT_0, ID_NEW_REPORT_0 + 99, OnNewReport)
  COMMAND_RANGE_HANDLER(ID_PAGE_0, ID_PAGE_0 + 99, OnPage)
  COMMAND_RANGE_HANDLER(ID_WIN_0, ID_WIN_0 + 99, OnWin)
  COMMAND_RANGE_HANDLER(ID_TRASH_0, ID_TRASH_0 + 99, OnTrash)
  COMMAND_RANGE_HANDLER(ID_FAV_TABLE_0, ID_FAV_TABLE_0 + 99, OnFavTable)
  COMMAND_RANGE_HANDLER(ID_FAV_GRAPH_0, ID_FAV_GRAPH_0 + 99, OnFavGraph)
  END_MSG_MAP()

  LRESULT OnNewDisplay(WORD /*wNotifyCode*/,
                       WORD /*wID*/,
                       HWND /*hWndCtl*/,
                       BOOL& /*bHandled*/);
  LRESULT OnNewReport(WORD /*wNotifyCode*/,
                      WORD /*wID*/,
                      HWND /*hWndCtl*/,
                      BOOL& /*bHandled*/);
  LRESULT OnPage(WORD /*wNotifyCode*/,
                 WORD /*wID*/,
                 HWND /*hWndCtl*/,
                 BOOL& /*bHandled*/);
  LRESULT OnWin(WORD /*wNotifyCode*/,
                WORD /*wID*/,
                HWND /*hWndCtl*/,
                BOOL& /*bHandled*/);
  LRESULT OnTrash(WORD /*wNotifyCode*/,
                  WORD /*wID*/,
                  HWND /*hWndCtl*/,
                  BOOL& /*bHandled*/);
  LRESULT OnFavTable(WORD /*wNotifyCode*/,
                     WORD /*wID*/,
                     HWND /*hWndCtl*/,
                     BOOL& /*bHandled*/);
  LRESULT OnFavGraph(WORD /*wNotifyCode*/,
                     WORD /*wID*/,
                     HWND /*hWndCtl*/,
                     BOOL& /*bHandled*/);

  // ui::SimpleMenuModel::Delegate
  virtual bool IsCommandIdChecked(int command_id) const override;
  virtual bool IsCommandIdEnabled(int command_id) const override;
  virtual bool GetAcceleratorForCommandId(
      int command_id,
      ui::Accelerator* accelerator) override;
  virtual void ExecuteCommand(int command_id) override;

  std::unique_ptr<ui::MenuModel> display_menu_;

  friend class MainMenu;
};
