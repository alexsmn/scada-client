#include "main_window/opened_view/opened_view_commands.h"

#include "aui/dialog_service.h"
#include "controller/controller.h"
#include "main_window/main_window.h"
#include "main_window/opened_view/opened_view.h"
#include "main_window/selection_commands.h"
#include "resources/common_resources.h"

#include <memory>
#include <utility>

OpenedViewCommands::OpenedViewCommands(OpenedViewCommandsContext&& context)
    : OpenedViewCommandsContext{std::move(context)} {}

OpenedViewCommands::~OpenedViewCommands() = default;

void OpenedViewCommands::SetContext(OpenedView* opened_view,
                                    DialogService* dialog_service) {
  assert(!opened_view || &opened_view->controller());

  opened_view_ = opened_view;
  main_window_ = opened_view ? &opened_view->main_window() : nullptr;
  dialog_service_ = dialog_service;
  controller_ = opened_view ? &opened_view->controller() : nullptr;
}

void OpenedViewCommands::AddCommandHandler(
    std::unique_ptr<CommandHandler> command_handler) {
  assert(command_handler);
  command_handlers_.emplace_back(std::move(command_handler));
}

CommandHandler* OpenedViewCommands::GetCommandHandler(unsigned command_id) {
  assert(controller_);

  if (auto* handler = controller_->GetCommandHandler(command_id)) {
    return handler;
  }

  if (auto* handler = selection_commands_->GetCommandHandler(command_id)) {
    return handler;
  }

  for (const auto& command_handler : command_handlers_) {
    if (auto* handler = command_handler->GetCommandHandler(command_id)) {
      return handler;
    }
  }

  return command_id == ID_VIEW_CLOSE ? this : nullptr;
}

void OpenedViewCommands::ExecuteCommand(unsigned command_id) {
  assert(opened_view_);

  switch (command_id) {
    case ID_VIEW_CLOSE:
      opened_view_->Close();
      return;
  }

  assert(false);
}

bool OpenedViewCommands::IsCommandChecked(unsigned command_id) const {
  return false;
}

bool OpenedViewCommands::IsCommandEnabled(unsigned command_id) const {
  return true;
}
