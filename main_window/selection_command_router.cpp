#include "main_window/selection_command_router.h"

#include "base/check.h"
#include "controller/selection_model.h"
#include "core/selection_command_context.h"
#include "main_window/main_window_interface.h"
#include "main_window/opened_view/opened_view.h"

#include <utility>

SelectionCommandRouter::SelectionCommandRouter(
    SelectionCommandRouterContext&& context)
    : SelectionCommandRouterContext{std::move(context)} {}

CommandHandler* SelectionCommandRouter::GetCommandHandler(unsigned command_id) {
  if (!selection_ || !dialog_service_) {
    return nullptr;
  }

  if (const auto* command = selection_commands_.FindCommand(command_id)) {
    if (!command->available_handler ||
        command->available_handler(command_context())) {
      return this;
    }
  }

  return nullptr;
}

void SelectionCommandRouter::SetContext(MainWindowInterface* main_window,
                                        DialogService* dialog_service,
                                        OpenedViewInterface* opened_view,
                                        SelectionModel* selection) {
  main_window_ = main_window;
  dialog_service_ = dialog_service;
  opened_view_ = opened_view;
  selection_ = selection;
}

bool SelectionCommandRouter::IsCommandEnabled(unsigned command_id) const {
  const auto* command = selection_commands_.FindCommand(command_id);
  return command && (!command->enabled_handler ||
                     command->enabled_handler(command_context()));
}

bool SelectionCommandRouter::IsCommandChecked(unsigned command_id) const {
  const auto* command = selection_commands_.FindCommand(command_id);
  return command && command->checked_handler &&
         command->checked_handler(command_context());
}

void SelectionCommandRouter::ExecuteCommand(unsigned command_id) {
  if (const auto* command = selection_commands_.FindCommand(command_id)) {
    if (command->execute_handler) {
      command->execute_handler(command_context());
    }
  }
}

SelectionCommandContext SelectionCommandRouter::command_context() const {
  base::Check(selection_);
  base::Check(dialog_service_);
  base::Check(main_window_);
  base::Check(opened_view_);

  return {.selection = *selection_,
          .dialog_service = *dialog_service_,
          .main_window = *main_window_,
          .opened_view = *opened_view_};
}
