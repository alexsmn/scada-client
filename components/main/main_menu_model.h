#pragma once

#include "base/files/file_path.h"
#include "services/file_cache.h"
#include "ui/base/models/simple_menu_model.h"

class ActionManager;
class CommandHandler;
class DialogService;
class Favourites;
class MainWindow;
class MainWindowManager;
class Profile;
class ViewManager;
class WindowDefinition;

struct MainMenuContext {
  MainWindowManager& main_window_manager_;
  MainWindow& main_window_;
  ActionManager& action_manager_;
  Favourites& favourites_;
  FileCache& file_cache_;
  const bool admin_;
  Profile& profile_;
  ViewManager& view_manager_;
  CommandHandler& command_handler_;
  DialogService& dialog_service_;
  ui::MenuModel& context_menu_model_;
};

class DisplayMenuModel : private MainMenuContext, public ui::SimpleMenuModel {
 public:
  explicit DisplayMenuModel(const MainMenuContext& context);

  // views::MenuModel
  virtual void MenuWillShow() override;
  virtual void ActivatedAt(int index) override;
  virtual bool IsEnabledAt(int index) const override;

 private:
  void AddItems(unsigned type);

  std::vector<base::FilePath> paths_;
};

class FavouritesMenuModel : private MainMenuContext,
                            public ui::SimpleMenuModel {
 public:
  FavouritesMenuModel(const unsigned view_types[],
                      const MainMenuContext& context);

  // views::MenuModel
  virtual void MenuWillShow() override;
  virtual void ActivatedAt(int index) override;
  virtual bool IsEnabledAt(int index) const override;

 private:
  const unsigned* view_types_;

  std::vector<const WindowDefinition*> windows_;
};

class PageMenuModel : private MainMenuContext, public ui::SimpleMenuModel {
 public:
  explicit PageMenuModel(const MainMenuContext& context);

  // views::MenuModel
  virtual void MenuWillShow() override;
  virtual void ActivatedAt(int index) override;
  virtual bool IsItemCheckedAt(int index) const override;

 private:
  int active_index_ = -1;
};

class WindowMenuModel : private MainMenuContext, public ui::SimpleMenuModel {
 public:
  explicit WindowMenuModel(const MainMenuContext& context)
      : MainMenuContext{context}, ui::SimpleMenuModel{nullptr} {}

  // views::MenuModel
  virtual void MenuWillShow() override;
  virtual void ActivatedAt(int index) override;
  virtual bool IsItemCheckedAt(int index) const override;

 private:
  int active_index_ = -1;
};

class TrashMenuModel : private MainMenuContext, public ui::SimpleMenuModel {
 public:
  explicit TrashMenuModel(const MainMenuContext& context)
      : MainMenuContext{context}, ui::SimpleMenuModel{nullptr} {}

  // views::MenuModel
  virtual void MenuWillShow() override;
  virtual void ActivatedAt(int index) override;
  virtual bool IsEnabledAt(int index) const override;

 private:
  bool empty_ = true;
};

class MainMenuModel final : private MainMenuContext,
                            private ui::SimpleMenuModel::Delegate,
                            public ui::SimpleMenuModel {
 public:
  explicit MainMenuModel(const MainMenuContext& context);

 private:
  void Rebuild();

  // ui::SimpleMenuModel::Delegate
  virtual bool IsCommandIdChecked(int command_id) const override;
  virtual bool IsCommandIdEnabled(int command_id) const override;
  virtual bool GetAcceleratorForCommandId(
      int command_id,
      ui::Accelerator* accelerator) override;
  virtual void ExecuteCommand(int command_id) override;

  DisplayMenuModel display_menu_model_;
  FavouritesMenuModel table_favourites_;
  ui::SimpleMenuModel table_submenu_;
  FavouritesMenuModel graph_favourites_;
  ui::SimpleMenuModel graph_submenu_;
  ui::SimpleMenuModel more_submenu_;
  PageMenuModel page_list_menu_;
  ui::SimpleMenuModel page_submenu_;
  WindowMenuModel window_list_menu_;
  TrashMenuModel trash_menu_;
  ui::SimpleMenuModel window_submenu_;
  ui::SimpleMenuModel settings_submenu_;
  ui::SimpleMenuModel help_submenu_;
};
