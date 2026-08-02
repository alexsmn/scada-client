#include "main_window/opened_view/opened_view_command_router.h"

#include "base/check.h"
#include "controller/controller.h"
#include "main_window/opened_view/opened_view.h"
#include "main_window/selection_command_router.h"
#include "resources/common_resources.h"

#include <memory>
#include <utility>

namespace {

class OpenedViewCloseCommand final : public CommandHandler {
 public:
  explicit OpenedViewCloseCommand(OpenedView& opened_view)
      : opened_view_{opened_view} {}

  virtual CommandHandler* GetCommandHandler(unsigned command_id) override {
    return command_id == ID_VIEW_CLOSE ? this : nullptr;
  }

  virtual void ExecuteCommand(unsigned command_id) override {
    scada::base::Check(command_id == ID_VIEW_CLOSE);
    opened_view_.Close();
  }

 private:
  OpenedView& opened_view_;
};

}  // namespace

OpenedViewCommandRouter::OpenedViewCommandRouter(
    OpenedViewCommandRouterContext&& context)
    : OpenedViewCommandRouterContext{std::move(context)} {}

OpenedViewCommandRouter::~OpenedViewCommandRouter() = default;

void OpenedViewCommandRouter::SetContext(OpenedView* opened_view) {
  scada::base::Check(!opened_view || &opened_view->controller());

  controller_ = opened_view ? &opened_view->controller() : nullptr;
  close_command_ = opened_view
                       ? std::make_unique<OpenedViewCloseCommand>(*opened_view)
                       : nullptr;
}

void OpenedViewCommandRouter::AddCommandHandler(
    std::unique_ptr<CommandHandler> command_handler) {
  scada::base::Check(command_handler);
  command_handlers_.emplace_back(std::move(command_handler));
}

void OpenedViewCommandRouter::AddCommandHandlers(
    std::vector<std::unique_ptr<CommandHandler>> command_handlers) {
  command_handlers_.reserve(command_handlers_.size() + command_handlers.size());
  for (auto& command_handler : command_handlers) {
    AddCommandHandler(std::move(command_handler));
  }
}

CommandHandler* OpenedViewCommandRouter::GetCommandHandler(
    unsigned command_id) {
  scada::base::Check(controller_);

  if (auto* handler = controller_->GetCommandHandler(command_id)) {
    return handler;
  }

  if (auto* handler =
          selection_command_router_->GetCommandHandler(command_id)) {
    return handler;
  }

  for (const auto& command_handler : command_handlers_) {
    if (auto* handler = command_handler->GetCommandHandler(command_id)) {
      return handler;
    }
  }

  return close_command_ ? close_command_->GetCommandHandler(command_id)
                        : nullptr;
}
