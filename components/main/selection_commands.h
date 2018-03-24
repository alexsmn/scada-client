#pragma once

#include "command_handler.h"

#include <vector>

namespace events {
class EventManager;
}

namespace scada {
class MethodService;
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
  MainWindow& main_window_;
  TaskManager& task_manager_;
  scada::MethodService& method_service_;
  scada::SessionService& session_service_;
  scada::NodeManagementService& node_management_service_;
  events::EventManager& event_manager_;
  TimedDataService& timed_data_service_;
  LocalEvents& local_events_;
  FileCache& file_cache_;
  Profile& profile_;
  DialogService& dialog_service_;
  MainWindowManager& main_window_manager_;
};

class SelectionCommands : private SelectionCommandsContext,
                          public CommandHandler {
 public:
  explicit SelectionCommands(SelectionCommandsContext&& context);

  void set_selection(SelectionModel* selection) { selection_ = selection; }

  void OpenWindow(unsigned type);

  // CommandHandler
  virtual CommandHandler* GetCommandHandler(unsigned command_id);
  virtual bool IsCommandEnabled(unsigned command_id) const;
  virtual bool IsCommandChecked(unsigned command_id) const;
  virtual void ExecuteCommand(unsigned command_id);

 private:
  void DoIOCtrl(const scada::NodeId& node_id,
                const scada::NodeId& method_id,
                const std::vector<scada::Variant>& arguments);
  void OpenModusView(const NodeRef& node);

  SelectionModel* selection_ = nullptr;
};
