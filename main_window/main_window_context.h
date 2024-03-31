#pragma once

#include "core/node_command_context.h"
#include "main_window/opened_view_factory.h"

#include <memory>
#include <string>

namespace aui {
class MenuModel;
class StatusBarModel;
}  // namespace aui

class ActionManager;
class CommandHandler;
class DialogService;
class Executor;
class FileManager;
class MainWindowInterface;
class MainWindowManager;
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
  OpenedViewFactory opened_view_factory_;

  std::function<std::unique_ptr<CommandHandler>(
      MainWindowInterface& main_window,
      DialogService& dialog_service)>
      main_commands_factory_;

  std::shared_ptr<SelectionCommands> selection_commands_;
  std::shared_ptr<aui::StatusBarModel> status_bar_model_;

  std::function<std::unique_ptr<aui::MenuModel>(
      MainWindowInterface& main_window,
      CommandHandler& command_handler)>
      context_menu_factory_;

  std::function<std::unique_ptr<aui::MenuModel>(
      MainWindowInterface& main_window,
      DialogService& dialog_service,
      ViewManager& view_manager,
      CommandHandler& command_handler,
      aui::MenuModel& context_menu_model)>
      main_menu_factory_;

  std::function<std::string()> connection_info_provider_;

  ProgressHost& progress_host_;
};
