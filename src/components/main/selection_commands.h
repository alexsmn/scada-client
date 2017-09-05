#pragma once

#include <functional>

#include "base/memory/weak_ptr.h"
#include "core/node_id.h"
#include "command_handler.h"

namespace base {
class FilePath;
}

namespace events {
class EventManager;
}

namespace scada {
class MethodService;
class NodeManagementService;
class SessionService;
}

class DialogService;
class FileCache;
class LocalEvents;
class MainWindow;
class NodeRefService;
class OpenedView;
class Profile;
class SelectionModel;
class TaskManager;
class TimedDataService;

struct SelectionCommandsContext {
  MainWindow* main_window_;
  TimedDataService& timed_data_service_;
  TaskManager& task_manager_;
  Profile& profile_;
  LocalEvents& local_events_;
  events::EventManager& event_manager_;
  scada::SessionService& session_service_;
  scada::NodeManagementService& node_management_service_;
  scada::MethodService& method_service_;
  FileCache& file_cache_;
  DialogService& dialog_service_;
  std::function<OpenedView*(const base::FilePath& path)> find_opened_view_;
  NodeRefService& node_service_;
};

class SelectionCommands : public CommandHandler,
                          private SelectionCommandsContext {
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
  void OpenModusView(const scada::NodeId& item_id);

  SelectionModel* selection_ = nullptr;

  base::WeakPtrFactory<SelectionCommands> weak_ptr_factory_{this};
};
