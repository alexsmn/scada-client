#pragma once

#include "ui/base/models/simple_menu_model.h"

class ActionManager;
class MainWindow;
class CommandHandler;

class ContextMenuModel final : public ui::SimpleMenuModel,
                               private ui::SimpleMenuModel::Delegate {
 public:
  ContextMenuModel(MainWindow& main_window,
                   ActionManager& action_manager,
                   CommandHandler& command_handler);

  // views::MenuModel
  virtual void MenuWillShow() override;

 private:
  void Rebuild();

  // ui::SimpleMenuModel::Delegate
  virtual bool IsCommandIdChecked(int command_id) const override;
  virtual bool IsCommandIdEnabled(int command_id) const override;
  virtual bool GetAcceleratorForCommandId(
      int command_id,
      ui::Accelerator* accelerator) override;
  virtual void ExecuteCommand(int command_id) override;

  MainWindow& main_window_;
  ActionManager& action_manager_;
  CommandHandler& command_handler_;

  std::vector<std::unique_ptr<ui::MenuModel>> submenus_;
};
