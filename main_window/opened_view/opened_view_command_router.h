#pragma once

#include "base/any_executor.h"

#include "controller/command_handler.h"

#include <memory>
#include <vector>

class Controller;
class OpenedView;
class SelectionCommandRouter;

struct OpenedViewCommandRouterContext {
  const AnyExecutor executor_;
  const std::shared_ptr<SelectionCommandRouter> selection_command_router_;
};

class OpenedViewCommandRouter : private OpenedViewCommandRouterContext,
                                public CommandHandler {
 public:
  explicit OpenedViewCommandRouter(OpenedViewCommandRouterContext&& context);
  ~OpenedViewCommandRouter();

  void SetContext(OpenedView* opened_view);
  // Adds a module-owned opened-view command handler.
  void AddCommandHandler(std::unique_ptr<CommandHandler> command_handler);
  void AddCommandHandlers(
      std::vector<std::unique_ptr<CommandHandler>> command_handlers);

  // CommandHandler
  virtual CommandHandler* GetCommandHandler(unsigned command_id) override;

 private:
  Controller* controller_ = nullptr;
  std::unique_ptr<CommandHandler> close_command_;
  std::vector<std::unique_ptr<CommandHandler>> command_handlers_;
};
