#pragma once

#include "aui/models/simple_menu_model.h"
#include "main_window/simple_menu_command_handler.h"

#include <memory>
#include <vector>

class ActionManager;
class MainWindowInterface;
class CommandHandler;

class ContextMenuModel final : public aui::SimpleMenuModel {
 public:
  ContextMenuModel(MainWindowInterface& main_window,
                   ActionManager& action_manager,
                   CommandHandler& command_handler);

  // views::MenuModel
  virtual void MenuWillShow() override;

 private:
  void Rebuild();

  MainWindowInterface& main_window_;
  ActionManager& action_manager_;

  SimpleMenuCommandHandler command_handler_;
  std::vector<std::unique_ptr<aui::MenuModel>> submenus_;
};
