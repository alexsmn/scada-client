#pragma once

#include "base/memory/scoped_vector.h"
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
  virtual bool IsCommandIdChecked(int command_id) const override {
    return false;
  }
  virtual bool IsCommandIdEnabled(int command_id) const override {
    return false;
  }
  virtual bool GetAcceleratorForCommandId(
      int command_id,
      ui::Accelerator* accelerator) override {
    return false;
  }
  virtual void ExecuteCommand(int command_id) override;

  MainWindow& main_window_;
  ActionManager& action_manager_;
  CommandHandler& command_handler_;

  ScopedVector<ui::MenuModel> submenus_;
};
