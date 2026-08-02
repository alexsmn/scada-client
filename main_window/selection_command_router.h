#pragma once

#include "controller/command_handler.h"
#include "controller/command_registry.h"

class DialogService;
class MainWindowInterface;
class OpenedViewInterface;
class SelectionModel;
struct SelectionCommandContext;

struct SelectionCommandRouterContext {
  BasicCommandRegistry<SelectionCommandContext>& selection_commands_;
};

// Adapts the active opened-view selection into `SelectionCommandContext` and
// routes matching commands from the shared selection command registry.
class SelectionCommandRouter : private SelectionCommandRouterContext,
                               public CommandHandler {
 public:
  explicit SelectionCommandRouter(SelectionCommandRouterContext&& context);

  SelectionModel* selection() { return selection_; }
  DialogService* dialog_service() { return dialog_service_; }
  MainWindowInterface* main_window() { return main_window_; }

  void SetContext(MainWindowInterface* main_window,
                  DialogService* dialog_service,
                  OpenedViewInterface* opened_view,
                  SelectionModel* selection);

  // CommandHandler
  virtual CommandHandler* GetCommandHandler(unsigned command_id) override;
  virtual bool IsCommandEnabled(unsigned command_id) const override;
  virtual std::u16string GetCommandDisabledReason(
      unsigned command_id) const override;
  virtual bool IsCommandChecked(unsigned command_id) const override;
  virtual void ExecuteCommand(unsigned command_id) override;

 private:
  SelectionCommandContext command_context() const;

  SelectionModel* selection_ = nullptr;
  MainWindowInterface* main_window_ = nullptr;
  OpenedViewInterface* opened_view_ = nullptr;
  DialogService* dialog_service_ = nullptr;
};
