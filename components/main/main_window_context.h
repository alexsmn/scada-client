#pragma once

#include "components/main/controller_factory.h"

#include <memory>
#include <string>

namespace ui {
class MenuModel;
}

class ActionManager;
class FileCache;
class MainWindow;
class MainWindowManager;
class OpenedView;
class Profile;
class StatusBarModel;
class ViewManager;

struct MainWindowContext {
  ActionManager& action_manager_;
  int window_id_;
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
  std::shared_ptr<StatusBarModel> status_bar_model_;
  std::function<std::unique_ptr<ui::MenuModel>(MainWindow& main_window,
                                               CommandHandler& main_commands)>
      context_menu_factory_;
  std::function<std::unique_ptr<ui::MenuModel>(
      MainWindow& main_window,
      DialogService& dialog_service,
      ViewManager& view_manager,
      CommandHandler& main_commands,
      ui::MenuModel& context_menu_model)>
      main_menu_factory_;
  std::function<std::string()> connection_info_provider_;
};
