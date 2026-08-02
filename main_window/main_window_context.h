#pragma once

#include "base/any_executor.h"

#include "core/node_command_context.h"
#include "main_window/opened_view/opened_view_factory.h"

#include <memory>
#include <string>

namespace scada::aui {
class MenuModel;
class StatusBarModel;
}  // namespace scada::aui

class ActionManager;
class CommandHandler;
class DialogService;
class FileManager;
class MainWindowInterface;
class MainWindowManager;
class NodeService;
class Profile;
class ProgressHost;
class SelectionCommandRouter;
class UiCommandRegistry;
class ViewManager;

struct MainWindowContext {
  AnyExecutor executor_;
  UiCommandRegistry& ui_command_registry_;
  int window_id_;
  NodeCommandHandler node_command_handler_;
  FileManager& file_manager_;
  MainWindowManager& main_window_manager_;
  Profile& profile_;
  OpenedViewFactory opened_view_factory_;

  std::function<std::unique_ptr<CommandHandler>(
      MainWindowInterface& main_window,
      DialogService& dialog_service)>
      main_command_router_factory_;

  std::shared_ptr<SelectionCommandRouter> selection_command_router_;
  std::shared_ptr<scada::aui::StatusBarModel> status_bar_model_;

  std::function<std::unique_ptr<scada::aui::MenuModel>(
      MainWindowInterface& main_window,
      CommandHandler& command_handler)>
      context_menu_factory_;

  std::function<std::unique_ptr<scada::aui::MenuModel>(
      MainWindowInterface& main_window,
      DialogService& dialog_service,
      ViewManager& view_manager,
      CommandHandler& command_handler,
      scada::aui::MenuModel& context_menu_model)>
      main_menu_factory_;

  std::function<std::string()> connection_info_provider_;

  ProgressHost& progress_host_;

  // Optional: the address-space service, used by the command palette's tag
  // search. Null in minimal/test contexts that do not exercise tag search.
  NodeService* node_service_ = nullptr;
};
