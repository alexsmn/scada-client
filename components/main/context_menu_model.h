#pragma once

#include "aui/models/simple_menu_model.h"
#include "simple_menu_command_handler.h"

class ActionManager;
class MainWindow;
class CommandHandler;

class ContextMenuModel final : public aui::SimpleMenuModel {
 public:
  ContextMenuModel(MainWindow& main_window,
                   ActionManager& action_manager,
                   CommandHandler& command_handler);

  // views::MenuModel
  virtual void MenuWillShow() override;

 private:
  void Rebuild();

  MainWindow& main_window_;
  ActionManager& action_manager_;

  SimpleMenuCommandHandler command_handler_;
  std::vector<std::unique_ptr<aui::MenuModel>> submenus_;
};
