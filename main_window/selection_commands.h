#pragma once

#include "base/cancelation.h"
#include "base/promise.h"
#include "controller/command_handler.h"
#include "controller/command_registry.h"
#include "profile/window_definition.h"

#include <vector>

namespace scada {
class SessionService;
}  // namespace scada

class Controller;
class DialogService;
class Executor;
class FileCache;
class MainWindowInterface;
class MainWindowManager;
class NodeEventProvider;
class NodeRef;
class NodeService;
class OpenedViewInterface;
class Profile;
class SelectionModel;
class TaskManager;
struct SelectionCommandContext;

struct SelectionCommandsContext {
  const std::shared_ptr<Executor> executor_;
  TaskManager& task_manager_;
  scada::SessionService& session_service_;
  NodeEventProvider& node_event_provider_;
  FileCache& file_cache_;
  Profile& profile_;
  MainWindowManager& main_window_manager_;
  NodeService& node_service_;
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

  void OpenWindow(const WindowInfo* window_info);
  void OpenWindow(const WindowDefinition& window_definition);

  // CommandHandler
  virtual CommandHandler* GetCommandHandler(unsigned command_id);
  virtual bool IsCommandEnabled(unsigned command_id) const override;
  virtual bool IsCommandChecked(unsigned command_id) const override;
  virtual void ExecuteCommand(unsigned command_id) override;

 private:
  SelectionCommandContext command_context() const;

  void DeleteSelection();
  void CopyToClipboard();

  promise<OpenedViewInterface*> OpenViewContainingNode(int view_type_id,
                                                       const NodeRef& node);

  SelectionModel* selection_ = nullptr;
  MainWindowInterface* main_window_ = nullptr;
  OpenedViewInterface* opened_view_ = nullptr;
  DialogService* dialog_service_ = nullptr;
  Controller* controller_ = nullptr;

  // TODO: Replace with |selection_commands_|.
  CommandRegistry command_registry_;

  Cancelation cancelation_;
};
