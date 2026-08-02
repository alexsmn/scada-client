#pragma once

#include "aui/models/simple_menu_model.h"
#include "main_window/simple_menu_command_handler.h"

#include <memory>
#include <vector>

class CommandManager;
class MainWindowInterface;
class CommandHandler;

class ContextMenuModel final : public scada::aui::SimpleMenuModel {
 public:
  ContextMenuModel(MainWindowInterface& main_window,
                   CommandManager& command_manager,
                   CommandHandler& command_handler);

  // views::MenuModel
  virtual void MenuWillShow() override;

 private:
  void Rebuild();

  MainWindowInterface& main_window_;
  CommandManager& command_manager_;

  SimpleMenuCommandHandler command_handler_;
  std::vector<std::unique_ptr<scada::aui::MenuModel>> submenus_;
};
