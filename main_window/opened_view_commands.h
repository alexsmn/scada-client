#pragma once

#include "aui/types.h"
#include "base/cancelation.h"
#include "base/promise.h"
#include "base/timer/timer.h"
#include "common/node_state.h"
#include "controller/command_handler.h"
#include "controller/controller_delegate.h"
#include "controller/controller_registry.h"
#include "profile/window_definition.h"
#include "scada/status.h"

#include <memory>
#include <string>

namespace scada {
class SessionService;
}

class ActionManager;
class Controller;
class CreateTree;
class DialogService;
class Executor;
class FileCache;
class LocalEvents;
class MainWindow;
class MainWindowManager;
class NodeService;
class OpenedView;
class Profile;
class SelectionCommands;
class TaskManager;
class TimedDataService;

struct OpenedViewCommandsContext {
  const std::shared_ptr<Executor> executor_;
  const std::shared_ptr<SelectionCommands> selection_commands_;
  TaskManager& task_manager_;
  scada::SessionService& session_service_;
  TimedDataService& timed_data_service_;
  NodeService& node_service_;
  ActionManager& action_manager_;
  LocalEvents& local_events_;
  FileCache& file_cache_;
  Profile& profile_;
  MainWindowManager& main_window_manager_;
  CreateTree& create_tree_;
};

class OpenedViewCommands : private OpenedViewCommandsContext,
                           public CommandHandler {
 public:
  explicit OpenedViewCommands(OpenedViewCommandsContext&& context);
  ~OpenedViewCommands();

  void SetContext(OpenedView* opened_view, DialogService* dialog_service);

  // CommandHandler
  virtual CommandHandler* GetCommandHandler(unsigned command_id) override;
  virtual void ExecuteCommand(unsigned command_id) override;
  virtual bool IsCommandChecked(unsigned command_id) const override;
  virtual bool IsCommandEnabled(unsigned command_id) const override;

 private:
  bool CanCreateRecord(const scada::NodeId& type_node_id) const;
  promise<> CreateRecord(const scada::NodeId& type_node_id, int tag);
  promise<> OnCreateRecordComplete(const scada::NodeId& node_id);

  promise<> PasteFromClipboard();

  void ExportToExcel();

  const bool excel_enabled_;

  OpenedView* opened_view_ = nullptr;
  Controller* controller_ = nullptr;
  MainWindow* main_window_ = nullptr;
  DialogService* dialog_service_ = nullptr;

  Cancelation cancelation_;
};
