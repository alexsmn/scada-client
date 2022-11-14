#pragma once

#include "components/main/controller_factory.h"

#include <memory>
#include <string>

namespace aui {
class MenuModel;
class StatusBarModel;
}

class ActionManager;
class FileCache;
class FileRegistry;
class Executor;
class MainWindow;
class MainWindowManager;
class OpenedView;
class Profile;
class SelectionCommands;
class ViewManager;

struct MainWindowContext {
  const std::shared_ptr<Executor> executor_;
  ActionManager& action_manager_;
  int window_id_;
  const FileRegistry& file_registry_;
  FileCache& file_cache_;
  MainWindowManager& main_window_manager_;
  Profile& profile_;
  ControllerFactory controller_factory_;
  std::function<std::unique_ptr<CommandHandler>(MainWindow& main_window,
                                                DialogService& dialog_service)>
      main_commands_factory_;
  std::function<std::unique_ptr<CommandHandler>(OpenedView& opened_view,
                                                DialogService& dialog_service)>
      view_commands_factory_;
  const std::shared_ptr<SelectionCommands> selection_commands_;
  std::shared_ptr<aui::StatusBarModel> status_bar_model_;
  std::function<std::unique_ptr<aui::MenuModel>(MainWindow& main_window,
                                               CommandHandler& main_commands)>
      context_menu_factory_;
  std::function<std::unique_ptr<aui::MenuModel>(
      MainWindow& main_window,
      DialogService& dialog_service,
      ViewManager& view_manager,
      CommandHandler& main_commands,
      aui::MenuModel& context_menu_model)>
      main_menu_factory_;
  std::function<std::string()> connection_info_provider_;
};
