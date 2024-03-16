#pragma once

#include "controller/controller_factory.h"
#include "core/node_command_context.h"

#include <memory>
#include <string>

namespace aui {
class MenuModel;
class StatusBarModel;
}  // namespace aui

class ActionManager;
class FileManager;
class Executor;
class MainWindow;
class MainWindowManager;
class OpenedView;
class Profile;
class ProgressHost;
class SelectionCommands;
class ViewManager;

struct MainWindowContext {
  std::shared_ptr<Executor> executor_;
  ActionManager& action_manager_;
  int window_id_;
  NodeCommandHandler node_command_handler_;
  FileManager& file_manager_;
  MainWindowManager& main_window_manager_;
  Profile& profile_;
  ControllerFactory controller_factory_;

  std::function<std::unique_ptr<CommandHandler>(MainWindow& main_window,
                                                DialogService& dialog_service)>
      main_commands_factory_;

  std::function<std::unique_ptr<CommandHandler>(OpenedView& opened_view,
                                                DialogService& dialog_service)>
      view_commands_factory_;

  std::shared_ptr<SelectionCommands> selection_commands_;
  std::shared_ptr<aui::StatusBarModel> status_bar_model_;

  std::function<std::unique_ptr<aui::MenuModel>(
      MainWindow& main_window,
      CommandHandler& command_handler)>
      context_menu_factory_;

  std::function<std::unique_ptr<aui::MenuModel>(
      MainWindow& main_window,
      DialogService& dialog_service,
      ViewManager& view_manager,
      CommandHandler& command_handler,
      aui::MenuModel& context_menu_model)>
      main_menu_factory_;

  std::function<std::string()> connection_info_provider_;

  ProgressHost& progress_host_;
};
