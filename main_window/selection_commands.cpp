#include "main_window/selection_commands.h"

#include "controller/selection_model.h"
#include "core/selection_command_context.h"
#include "main_window/main_window_interface.h"
#include "main_window/opened_view/opened_view.h"

#include <utility>

SelectionCommands::SelectionCommands(SelectionCommandsContext&& context)
    : SelectionCommandsContext{std::move(context)} {}

CommandHandler* SelectionCommands::GetCommandHandler(unsigned command_id) {
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

void SelectionCommands::SetContext(MainWindowInterface* main_window,
                                   DialogService* dialog_service,
                                   OpenedViewInterface* opened_view,
                                   Controller* controller,
                                   SelectionModel* selection) {
  main_window_ = main_window;
  dialog_service_ = dialog_service;
  opened_view_ = opened_view;
  controller_ = controller;
  selection_ = selection;
}

bool SelectionCommands::IsCommandEnabled(unsigned command_id) const {
  const auto* command = selection_commands_.FindCommand(command_id);
  return command && (!command->enabled_handler ||
                     command->enabled_handler(command_context()));
}

bool SelectionCommands::IsCommandChecked(unsigned command_id) const {
  const auto* command = selection_commands_.FindCommand(command_id);
  return command && command->checked_handler &&
         command->checked_handler(command_context());
}

void SelectionCommands::ExecuteCommand(unsigned command_id) {
  if (const auto* command = selection_commands_.FindCommand(command_id)) {
    if (command->execute_handler) {
      command->execute_handler(command_context());
    }
  }
}

SelectionCommandContext SelectionCommands::command_context() const {
  assert(selection_);
  assert(dialog_service_);
  assert(main_window_);
  assert(opened_view_);

  return {.selection = *selection_,
          .dialog_service = *dialog_service_,
          .main_window = *main_window_,
          .opened_view = *opened_view_};
}
