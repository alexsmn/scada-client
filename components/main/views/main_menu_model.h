#pragma once

#include "base/files/file_path.h"
#include "services/file_cache.h"
#include "ui/base/models/simple_menu_model.h"

class ActionManager;
class CommandHandler;
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
};

class InplaceMenuModel : public ui::SimpleMenuModel {
 public:
  explicit InplaceMenuModel(ui::SimpleMenuModel::Delegate* delegate);

  void AddInplaceMenu(std::shared_ptr<ui::MenuModel> model);

  // views::MenuModel
  virtual void MenuWillShow() override;
  virtual void ActivatedAt(int index) override;
  virtual bool IsEnabledAt(int index) const override;

 private:
  struct InplaceInfo {
    const int index;
    const std::shared_ptr<ui::MenuModel> model;
    int count;
    int offset;
  };

  std::pair<ui::MenuModel*, int> GetModelAndIndexAt(int index);
  std::pair<const ui::MenuModel*, int> GetModelAndIndexAt(int index) const;

  std::vector<InplaceInfo> inplace_models_;
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
  std::unique_ptr<ui::MenuModel> context_menu_;
  InplaceMenuModel table_submenu_;
  InplaceMenuModel graph_submenu_;
  ui::SimpleMenuModel more_submenu_;
  InplaceMenuModel page_submenu_;
  InplaceMenuModel window_submenu_;
  ui::SimpleMenuModel settings_submenu_;
  ui::SimpleMenuModel help_submenu_;
};
