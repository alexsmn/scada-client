#pragma once

#include "command_handler.h"

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

class DialogService;
class FileCache;
class LocalEvents;
class MainWindow;
class MainWindowManager;
class NodeRef;
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
};

class SelectionCommands : private SelectionCommandsContext,
                          public CommandHandler {
 public:
  explicit SelectionCommands(SelectionCommandsContext&& context);

  void SetContext(MainWindow* main_window,
                  DialogService* dialog_service,
                  SelectionModel* selection);

  void OpenWindow(unsigned type);

  // CommandHandler
  virtual CommandHandler* GetCommandHandler(unsigned command_id);
  virtual bool IsCommandEnabled(unsigned command_id) const;
  virtual bool IsCommandChecked(unsigned command_id) const;
  virtual void ExecuteCommand(unsigned command_id);

 private:
  void CallMethod(const NodeRef& node,
                  const scada::NodeId& method_id,
                  const std::vector<scada::Variant>& arguments);
  void OpenModusView(const NodeRef& node);

  SelectionModel* selection_ = nullptr;
  MainWindow* main_window_ = nullptr;
  DialogService* dialog_service_ = nullptr;
};
