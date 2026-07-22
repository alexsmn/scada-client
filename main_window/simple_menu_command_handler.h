#pragma once

#include "aui/models/simple_menu_model.h"
#include "controller/command_handler.h"

class SimpleMenuCommandHandler : public scada::aui::SimpleMenuModel::Delegate {
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

  virtual std::u16string GetDisabledReasonForCommandId(
      int command_id) const override {
    auto* handler = commands_.GetCommandHandler(command_id);
    return handler ? handler->GetCommandDisabledReason(command_id)
                   : std::u16string{};
  }

  virtual void ExecuteCommand(int command_id) override {
    if (auto* handler = commands_.GetCommandHandler(command_id))
      handler->ExecuteCommand(command_id);
  }

  CommandHandler& commands_;
};
