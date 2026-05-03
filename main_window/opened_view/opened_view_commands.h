#pragma once

#include "base/any_executor.h"

#include "aui/types.h"
#include "base/awaitable.h"
#include "base/cancelation.h"
#include "base/timer/timer.h"
#include "common/node_state.h"
#include "controller/controller_delegate.h"
#include "controller/controller_registry.h"
#include "profile/window_definition.h"
#include "scada/status.h"

#include <memory>
#include <string>

namespace scada {
class SessionService;
}

struct Action;
class ActionManager;
class Controller;
class CreateTree;
class DialogService;
class FileCache;
class LocalEvents;
class MainWindow;
class MainWindowManager;
class NodeService;
class OpenedView;
class Profile;
class PrintService;
class SelectionCommands;
class TaskManager;
class TimedDataService;
struct SelectionCommandContext;

struct OpenedViewCommandsContext {
  const AnyExecutor executor_;
  const std::shared_ptr<SelectionCommands> selection_commands_;
  ActionManager& action_manager_;
  TaskManager& task_manager_;
  scada::SessionService& session_service_;
  TimedDataService& timed_data_service_;
  NodeService& node_service_;
  PrintService* print_service_;
  LocalEvents& local_events_;
  FileCache& file_cache_;
  Profile& profile_;
  MainWindowManager& main_window_manager_;
  CreateTree& create_tree_;
};

class OpenedViewCommands : private OpenedViewCommandsContext {
 public:
  explicit OpenedViewCommands(OpenedViewCommandsContext&& context);
  ~OpenedViewCommands();

  void SetContext(OpenedView* opened_view, DialogService* dialog_service);

  Action* FindAction(unsigned command_id) const;
  void ExecuteAction(unsigned command_id);
  bool IsActionChecked(unsigned command_id) const;
  bool IsActionEnabled(unsigned command_id) const;

 private:
  SelectionCommandContext selection_command_context() const;

  bool CanCreateRecord(const scada::NodeId& type_node_id) const;
  // Spawns the create + report + open-view coroutine and returns immediately.
  // The coroutine lifetime is gated by `cancelation_` so it cannot outlive
  // `this`.
  void CreateRecord(const scada::NodeId& type_node_id, int tag);

  Awaitable<void> CreateRecordAsync(scada::NodeId type_node_id,
                                    scada::NodeId parent_id,
                                    std::u16string title,
                                    scada::NodeAttributes attributes,
                                    scada::NodeProperties properties);
  Awaitable<void> OnCreateRecordCompleteAsync(scada::NodeId node_id);
  Awaitable<void> PasteFromClipboardAsync();

  Awaitable<void> PasteFromClipboard();

  void ExportToExcel();

  const bool excel_enabled_;

  OpenedView* opened_view_ = nullptr;
  Controller* controller_ = nullptr;
  MainWindow* main_window_ = nullptr;
  DialogService* dialog_service_ = nullptr;

  Cancelation cancelation_;
};
