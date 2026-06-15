#include "main_window/main_window_command_router.h"

#include "aui/prompt_dialog.h"
#include "aui/translation.h"
#include "base/awaitable.h"
#include "controller/command_registry.h"
#include "controller/window_info.h"
#include "main_window/main_window.h"
#include "main_window/main_window_manager.h"
#include "main_window/opened_view/opened_view.h"
#include "main_window/opened_view/opened_view_command_router.h"
#include "main_window/standard_command_ids.h"
#include "net/net_executor_adapter.h"
#include "resources/common_resources.h"
#include "scada/session_service.h"
#include "ui/common/client_utils.h"

MainWindowCommandRouter::MainWindowCommandRouter(
    MainWindowCommandRouterContext&& context)
    : MainWindowCommandRouterContext{std::move(context)},
      command_context_{main_window_, dialog_service_} {}

MainWindowCommandRouter::~MainWindowCommandRouter() {}

CommandHandler* MainWindowCommandRouter::GetCommandHandler(
    unsigned command_id) {
  auto* active_view = main_window_.GetActiveView();
  if (active_view) {
    // TODO: Refactor to remove the static cast.
    if (auto* handler = static_cast<OpenedView*>(active_view)
                            ->commands->GetCommandHandler(command_id)) {
      return handler;
    }
  }

  switch (command_id) {
    case ID_WINDOW_NEW:
    case ID_VIEW_ADD_TO_FAVOURITES:
    case ID_VIEW_CHANGE_TITLE:
#if defined(UI_QT)
    case ID_WINDOW_SPLIT_HORZ:
    case ID_WINDOW_SPLIT_VERT:
#endif
      return active_view ? this : nullptr;

      /*case ID_PRINT:
        return active_view && active_view->window_info().printable() ? this
                                                                     :
        nullptr;*/

    case ID_LOGIN:
    case ID_LOGOFF:
      return this;

    case ID_OPEN_TABLE:
      command_id = ID_TABLE_VIEW;
      break;
    case ID_OPEN_GRAPH:
      command_id = ID_GRAPH_VIEW;
      break;
    case ID_TIMED_DATA_VIEW:
      command_id = ID_TIMED_DATA_VIEW;
      break;
    case ID_OPEN_EVENTS:
      command_id = ID_EVENT_JOURNAL_VIEW;
      break;
  }

  if (const WindowInfo* win_info = FindWindowInfo(command_id)) {
    if (!win_info->createable()) {
      return nullptr;
    }
    if (win_info->requires_admin_rights() &&
        !session_service_.HasPrivilege(scada::Privilege::Configure)) {
      return nullptr;
    }
    return this;
  }

  if (const auto* command = global_commands_.FindCommand(command_id)) {
    return !command->available_handler ||
                   command->available_handler(command_context_)
               ? this
               : nullptr;
  }

  return nullptr;
}

bool MainWindowCommandRouter::IsCommandEnabled(unsigned command_id) const {
  auto* active_view = main_window_.GetActiveView();

  switch (command_id) {
    case ID_VIEW_ADD_TO_FAVOURITES:
    case ID_VIEW_CHANGE_TITLE:
      return active_view && !active_view->GetWindowInfo().is_pane();

    case ID_LOGIN:
      return !session_service_.IsConnected();
    case ID_LOGOFF:
      return session_service_.IsConnected();
  }

  if (const auto* command = global_commands_.FindCommand(command_id)) {
    return !command->enabled_handler ||
           command->enabled_handler(command_context_);
  }

  return true;
}

bool MainWindowCommandRouter::IsCommandChecked(unsigned command_id) const {
  if (const WindowInfo* window_info = FindWindowInfo(command_id)) {
    return (window_info->flags & WIN_SING) &&
           main_window_.FindViewByType(window_info->name);
  }

  if (const auto* command = global_commands_.FindCommand(command_id)) {
    return command->checked_handler &&
           command->checked_handler(command_context_);
  }

  return false;
}

void MainWindowCommandRouter::ExecuteCommand(unsigned command_id) {
  switch (command_id) {
    case ID_WINDOW_NEW:
      main_window_manager_.CreateMainWindow();
      return;

    case ID_VIEW_CHANGE_TITLE:
      ShowRenameWindowDialog();
      return;

    case ID_LOGIN:
    case ID_LOGOFF:
      login_handler_(command_id == ID_LOGIN);
      return;

#if defined(UI_QT)
    case ID_WINDOW_SPLIT_HORZ:
    case ID_WINDOW_SPLIT_VERT:
      if (auto* active_view = main_window_.GetActiveView()) {
        main_window_.SplitView(*active_view,
                               command_id == ID_WINDOW_SPLIT_HORZ);
      }
      return;
#endif

    case ID_OPEN_TABLE:
      command_id = ID_TABLE_VIEW;
      break;
    case ID_OPEN_GRAPH:
      command_id = ID_GRAPH_VIEW;
      break;
    case ID_TIMED_DATA_VIEW:
      command_id = ID_TIMED_DATA_VIEW;
      break;
    case ID_OPEN_EVENTS:
      command_id = ID_EVENT_JOURNAL_VIEW;
      break;
  }

  // Check create window command.
  if (const WindowInfo* win_info = FindWindowInfo(command_id)) {
    assert(win_info->createable());
    /*if (win_info->flags & WIN_SING) {
      OpenedView* view = view_manager_->FindViewByType(win_info->type);
      if (view) {
        view_manager_->CloseView(*view);
        return;
      }
    }*/
    CoSpawn(executor_,
            [this, def = WindowDefinition(*win_info)]() -> Awaitable<void> {
              co_await main_window_.OpenView(def, true);
            });
    return;
  }

  if (const auto* command = global_commands_.FindCommand(command_id)) {
    if (command->execute_handler) {
      command->execute_handler(command_context_);
    }
    return;
  }

  assert(false);
}

namespace {

Awaitable<void> ShowRenameWindowDialogAsync(AnyExecutor executor,
                                            OpenedViewInterface& view,
                                            DialogService& dialog_service,
                                            std::u16string current_view_title) {
  auto title =
      co_await RunPromptDialog(dialog_service, Translate("Name:"),
                               Translate("Rename"), current_view_title);
  // TODO: Capture weak pointer.
  view.SetWindowTitle(title);
  co_return;
}

}  // namespace

void MainWindowCommandRouter::ShowRenameWindowDialog() {
  auto* view = main_window_.GetActiveView();
  if (!view || view->GetWindowInfo().is_pane()) {
    return;
  }

  CoSpawn(executor_,
          [executor = executor_, view, &dialog_service = dialog_service_,
           current_view_title = view->GetWindowTitle()] {
            return ShowRenameWindowDialogAsync(executor, *view, dialog_service,
                                               current_view_title);
          });
}
