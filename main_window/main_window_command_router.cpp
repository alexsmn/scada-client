#include "main_window/main_window_command_router.h"

#include "base/awaitable.h"
#include "base/check.h"
#include "controller/command_registry.h"
#include "controller/window_info.h"
#include "main_window/main_window.h"
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

  if (const auto* command = global_commands_.FindCommand(command_id)) {
    return !command->available_handler ||
                   command->available_handler(command_context_)
               ? this
               : nullptr;
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

  return nullptr;
}

bool MainWindowCommandRouter::IsCommandEnabled(unsigned command_id) const {
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
  if (const auto* command = global_commands_.FindCommand(command_id)) {
    if (command->execute_handler) {
      command->execute_handler(command_context_);
    }
    return;
  }

  // Check create window command.
  if (const WindowInfo* win_info = FindWindowInfo(command_id)) {
    scada::base::Check(win_info->createable());
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

  scada::base::NotReached();
}
