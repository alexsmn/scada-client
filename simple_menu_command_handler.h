#pragma once

#include "command_handler.h"
#include "controls/models/simple_menu_model.h"

class SimpleMenuCommandHandler : public aui::SimpleMenuModel::Delegate {
 public:
  explicit SimpleMenuCommandHandler(CommandHandler& commands)
      : commands_{commands} {}

  // ui::SimpleMenuModel::Delegate
  virtual bool IsCommandIdChecked(int command_id) const override {
    auto* handler = commands_.GetCommandHandler(command_id);
    return handler && handler->IsCommandChecked(command_id);
  }

  virtual bool IsCommandIdEnabled(int command_id) const override {
    auto* handler = commands_.GetCommandHandler(command_id);
    return handler && handler->IsCommandEnabled(command_id);
  }

  virtual void ExecuteCommand(int command_id) override {
    if (auto* handler = commands_.GetCommandHandler(command_id))
      handler->ExecuteCommand(command_id);
  }

  CommandHandler& commands_;
};
