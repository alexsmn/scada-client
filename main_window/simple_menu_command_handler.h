#pragma once

#include "aui/models/simple_menu_model.h"
#include "main_window/main_window_commands.h"

class SimpleMenuCommandHandler : public aui::SimpleMenuModel::Delegate {
 public:
  explicit SimpleMenuCommandHandler(MainWindowCommandHandler& commands)
      : commands_{commands} {}

  // ui::SimpleMenuModel::Delegate
  virtual bool IsCommandIdChecked(int command_id) const override {
    return commands_.FindAction(command_id) &&
           commands_.IsActionChecked(command_id);
  }

  virtual bool IsCommandIdEnabled(int command_id) const override {
    return commands_.FindAction(command_id) &&
           commands_.IsActionEnabled(command_id);
  }

  virtual void ExecuteCommand(int command_id) override {
    if (commands_.FindAction(command_id))
      commands_.ExecuteAction(command_id);
  }

  MainWindowCommandHandler& commands_;
};
