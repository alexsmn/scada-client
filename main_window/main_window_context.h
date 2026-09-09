#pragma once

#include "base/any_executor.h"

#include "core/node_command_context.h"
#include "main_window/opened_view/opened_view_factory.h"

#include <functional>
#include <memory>
#include <string>

namespace scada::aui {
class MenuModel;
class StatusBarModel;
}  // namespace scada::aui

namespace scada {
class AttributeService;
class NodeId;
class SessionService;
}  // namespace scada

class ActionManager;
class CommandHandler;
class DialogService;
class FileManager;
class MainWindowInterface;
class MainWindowManager;
class NodeRef;
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

  // Optional: raw attribute reads, for attributes `NodeService` does not
  // fetch. The RBAC inspector needs it to read the server's role -> permission
  // map (the RolePermissions attribute, OPC UA Part 3 §5.2.9) rather than
  // carry its own copy of it. Null in minimal/test contexts, and the panel
  // then reports the permissions as unknown rather than assuming a map.
  scada::AttributeService* attribute_service_ = nullptr;

  // Optional: the session, watched only to notice a re-login. The window
  // survives one (OnLoginCompleted swaps the services under it), so anything
  // it cached from the previous session -- the command palette's tag index --
  // has to be dropped when a new one comes up. Null in minimal/test contexts.
  scada::SessionService* session_service_ = nullptr;

  // Optional: calls an OPC UA Method on a node, reporting progress and result
  // through the task manager. Used by the device-diagnostics panel's
  // protocol link action (ADR 0007's Reconnect). Null in minimal/test
  // contexts, and the panel then draws no such button at all rather than a
  // dead one.
  std::function<void(const NodeRef& node, const scada::NodeId& method_id)>
      call_node_method_;

  // Optional: whether this session holds the OPC UA Call permission
  // (PermissionType.Call, Part 3 §8.55). Null means "assume it does". The
  // server is the authority regardless; this only decides whether a control
  // button is offered live or disabled with its reason.
  std::function<bool()> has_call_permission_;
};
