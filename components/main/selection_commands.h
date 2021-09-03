#pragma once

#include "base/memory/weak_ptr.h"
#include "base/promise.h"
#include "command_handler.h"
#include "command_registry.h"
#include "window_definition.h"

#include <vector>

namespace scada {
class NodeId;
class NodeManagementService;
class SessionService;
class Variant;
}  // namespace scada

class Controller;
class DialogService;
class EventFetcher;
class Executor;
class FileCache;
class LocalEvents;
class MainWindow;
class MainWindowManager;
class NodeRef;
class NodeService;
class Profile;
class SelectionModel;
class TaskManager;
class TimedDataService;

struct SelectionCommandsContext {
  const std::shared_ptr<Executor> executor_;
  TaskManager& task_manager_;
  scada::SessionService& session_service_;
  scada::NodeManagementService& node_management_service_;
  EventFetcher& event_fetcher_;
  TimedDataService& timed_data_service_;
  LocalEvents& local_events_;
  FileCache& file_cache_;
  Profile& profile_;
  MainWindowManager& main_window_manager_;
  NodeService& node_service_;
};

// A singleton shared between |OpenedView|s. Once an |OpenView| is focused, it
// calls |SetContext|.
class SelectionCommands : private SelectionCommandsContext,
                          public CommandHandler {
 public:
  explicit SelectionCommands(SelectionCommandsContext&& context);

  SelectionModel* selection() { return selection_; }
  DialogService* dialog_service() { return dialog_service_; }

  void SetContext(MainWindow* main_window,
                  DialogService* dialog_service,
                  Controller* controller,
                  SelectionModel* selection);

  // Selection API

  NodeRef GetSelectedNode();

  void OpenWindow(const WindowInfo* window_info);
  void OpenWindow(const WindowDefinition& window_definition);

  // CommandHandler
  virtual CommandHandler* GetCommandHandler(unsigned command_id);
  virtual bool IsCommandEnabled(unsigned command_id) const;
  virtual bool IsCommandChecked(unsigned command_id) const;
  virtual void ExecuteCommand(unsigned command_id);

 private:
  void DeleteSelection();
  void CopyToClipboard();

  void CallMethod(const NodeRef& node,
                  const scada::NodeId& method_id,
                  const std::vector<scada::Variant>& arguments);
  void OpenModusView(const NodeRef& node);

  promise<WindowDefinition> GetOpenWindowDefinition(
      const WindowInfo* window_info) const;

  void DumpDebugInfo();

  SelectionModel* selection_ = nullptr;
  MainWindow* main_window_ = nullptr;
  DialogService* dialog_service_ = nullptr;
  Controller* controller_ = nullptr;

  CommandRegistry command_registry_;

  base::WeakPtrFactory<SelectionCommands> weak_ptr_factory_{this};
};
