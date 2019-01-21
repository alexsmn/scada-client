#pragma once

#include "command_handler.h"
#include "window_definition.h"

#include <vector>

namespace events {
class EventManager;
}

namespace scada {
class NodeId;
class NodeManagementService;
class SessionService;
class Variant;
}  // namespace scada

class Controller;
class DialogService;
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
  TaskManager& task_manager_;
  scada::SessionService& session_service_;
  scada::NodeManagementService& node_management_service_;
  events::EventManager& event_manager_;
  TimedDataService& timed_data_service_;
  LocalEvents& local_events_;
  FileCache& file_cache_;
  Profile& profile_;
  MainWindowManager& main_window_manager_;
  NodeService& node_service_;
};

class SelectionCommands : private SelectionCommandsContext,
                          public CommandHandler {
 public:
  explicit SelectionCommands(SelectionCommandsContext&& context);

  void SetContext(MainWindow* main_window,
                  DialogService* dialog_service,
                  Controller* controller,
                  SelectionModel* selection);

  void OpenWindow(unsigned type);

  // CommandHandler
  virtual CommandHandler* GetCommandHandler(unsigned command_id);
  virtual bool IsCommandEnabled(unsigned command_id) const;
  virtual bool IsCommandChecked(unsigned command_id) const;
  virtual void ExecuteCommand(unsigned command_id);

 private:
  void ExecuteMultiCommand(unsigned command_id);

  void DeleteSelection();
  void CopyToClipboard();

  void CallMethod(const NodeRef& node,
                  const scada::NodeId& method_id,
                  const std::vector<scada::Variant>& arguments);
  void OpenModusView(const NodeRef& node);

  WindowDefinition GetOpenWindowDefinition(unsigned type) const;

  void DumpDebugInfo();

  SelectionModel* selection_ = nullptr;
  MainWindow* main_window_ = nullptr;
  DialogService* dialog_service_ = nullptr;
  Controller* controller_ = nullptr;
};
