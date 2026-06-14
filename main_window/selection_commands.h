#pragma once

#include "controller/command_handler.h"
#include "controller/command_registry.h"

class Controller;
class DialogService;
class MainWindowInterface;
class NodeRef;
class OpenedViewInterface;
class SelectionModel;
struct SelectionCommandContext;

struct SelectionCommandsContext {
  BasicCommandRegistry<SelectionCommandContext>& selection_commands_;
};

// A singleton shared between |OpenedView|s. Once an |OpenView| is focused, it
// calls |SetContext|.
class SelectionCommands : private SelectionCommandsContext,
                          public CommandHandler {
 public:
  explicit SelectionCommands(SelectionCommandsContext&& context);

  SelectionModel* selection() { return selection_; }
  DialogService* dialog_service() { return dialog_service_; }
  MainWindowInterface* main_window() { return main_window_; }

  void SetContext(MainWindowInterface* main_window,
                  DialogService* dialog_service,
                  OpenedViewInterface* opened_view,
                  Controller* controller,
                  SelectionModel* selection);

  // CommandHandler
  virtual CommandHandler* GetCommandHandler(unsigned command_id) override;
  virtual bool IsCommandEnabled(unsigned command_id) const override;
  virtual bool IsCommandChecked(unsigned command_id) const override;
  virtual void ExecuteCommand(unsigned command_id) override;

 private:
  SelectionCommandContext command_context() const;

  SelectionModel* selection_ = nullptr;
  MainWindowInterface* main_window_ = nullptr;
  OpenedViewInterface* opened_view_ = nullptr;
  DialogService* dialog_service_ = nullptr;
  Controller* controller_ = nullptr;
};
