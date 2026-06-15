#pragma once

#include "base/any_executor.h"

#include "controller/command_handler.h"

#include <memory>
#include <vector>

namespace scada {
class SessionService;
}

class ActionManager;
class Controller;
class DialogService;
class MainWindow;
class OpenedView;
class SelectionCommands;

struct OpenedViewCommandsContext {
  const AnyExecutor executor_;
  const std::shared_ptr<SelectionCommands> selection_commands_;
};

class OpenedViewCommands : private OpenedViewCommandsContext,
                           public CommandHandler {
 public:
  explicit OpenedViewCommands(OpenedViewCommandsContext&& context);
  ~OpenedViewCommands();

  void SetContext(OpenedView* opened_view, DialogService* dialog_service);
  // Adds a module-owned opened-view command handler.
  void AddCommandHandler(std::unique_ptr<CommandHandler> command_handler);

  // CommandHandler
  virtual CommandHandler* GetCommandHandler(unsigned command_id) override;
  virtual void ExecuteCommand(unsigned command_id) override;
  virtual bool IsCommandChecked(unsigned command_id) const override;
  virtual bool IsCommandEnabled(unsigned command_id) const override;

 private:
  OpenedView* opened_view_ = nullptr;
  Controller* controller_ = nullptr;
  MainWindow* main_window_ = nullptr;
  DialogService* dialog_service_ = nullptr;

  std::vector<std::unique_ptr<CommandHandler>> command_handlers_;
};
