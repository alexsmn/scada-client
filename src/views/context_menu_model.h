#pragma once

#include "base/memory/scoped_vector.h"
#include "ui/base/models/simple_menu_model.h"

class ActionManager;
class MainWindowViews;

class ContextMenuModel : public ui::SimpleMenuModel,
                         private ui::SimpleMenuModel::Delegate {
 public:
  ContextMenuModel(MainWindowViews* main_view, ActionManager& action_manager);

  // views::MenuModel
  virtual void MenuWillShow() override;

 private:
  void Rebuild();

  // ui::SimpleMenuModel::Delegate
  virtual bool IsCommandIdChecked(int command_id) const override { return false; }
  virtual bool IsCommandIdEnabled(int command_id) const override { return false; }
  virtual bool GetAcceleratorForCommandId(
      int command_id,
      ui::Accelerator* accelerator) override { return false; }
  virtual void ExecuteCommand(int command_id) override;

  MainWindowViews* main_view_;
  ActionManager& action_manager_;

  ScopedVector<ui::MenuModel> submenus_;
};
