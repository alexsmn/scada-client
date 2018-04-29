#pragma once

#include <memory>

#include "base/memory/weak_ptr.h"
#include "base/strings/string16.h"
#include "base/timer/timer.h"
#include "command_handler.h"
#include "controller_delegate.h"
#include "controller_factory.h"
#include "controls/types.h"
#include "core/configuration_types.h"
#include "core/status.h"
#include "window_definition.h"

namespace events {
class EventManager;
}

namespace scada {
class HistoryService;
class MethodService;
class MonitoredItemService;
class NodeManagementService;
class SessionService;
}  // namespace scada

class ActionManager;
class ContentsModel;
class Controller;
class DialogService;
class Favourites;
class FileCache;
class LocalEvents;
class MainWindow;
class MainWindowManager;
class NodeService;
class OpenedView;
class PortfolioManager;
class Profile;
class SelectionCommands;
class TaskManager;
class TimedDataService;

struct OpenedViewCommandsContext {
  TaskManager& task_manager_;
  scada::MethodService& method_service_;
  scada::SessionService& session_service_;
  scada::NodeManagementService& node_management_service_;
  events::EventManager& event_manager_;
  scada::HistoryService& history_service_;
  scada::MonitoredItemService& monitored_item_service_;
  TimedDataService& timed_data_service_;
  NodeService& node_service_;
  PortfolioManager& portfolio_manager_;
  ActionManager& action_manager_;
  LocalEvents& local_events_;
  Favourites& favourites_;
  FileCache& file_cache_;
  Profile& profile_;
  MainWindowManager& main_window_manager_;
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

 private:
  bool CanCreateRecord(const scada::NodeId& type_node_id) const;
  void CreateRecord(const scada::NodeId& type_node_id, int tag);
  void OnCreateRecordComplete(const scada::LocalizedText& display_name,
                              const scada::Status& status,
                              const scada::NodeId& node_id);

  OpenedView* opened_view_ = nullptr;
  Controller* controller_ = nullptr;
  MainWindow* main_window_ = nullptr;
  DialogService* dialog_service_ = nullptr;

  std::unique_ptr<SelectionCommands> selection_commands_;

  base::WeakPtrFactory<OpenedViewCommands> weak_factory_{this};
};
