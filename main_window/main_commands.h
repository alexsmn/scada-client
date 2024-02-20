#pragma once

#include "base/promise.h"
#include "controller/command_handler.h"
#include "controller/command_registry.h"
#include "core/main_command_context.h"

#include <functional>
#include <memory>
#include <type_traits>

namespace scada {
class SessionService;
}

class Debugger;
class DialogService;
class Favourites;
class LocalEvents;
class MainWindow;
class MainWindowManager;
class NodeEventProvider;
class NodeService;
class Profile;
class Speech;
class TaskManager;

struct MainCommandsContext {
  MainWindow& main_window_;
  TaskManager& task_manager_;
  DialogService& dialog_service_;
  scada::SessionService& session_service_;
  NodeEventProvider& node_event_provider_;
  NodeService& node_service_;
  LocalEvents& local_events_;
  Favourites& favourites_;
  Speech& speech_;
  Profile& profile_;
  MainWindowManager& main_window_manager_;
  std::function<void(bool login)> login_handler_;
  Debugger& debugger_;
  const BasicCommandRegistry<MainCommandContext>& main_commands_;
};

class MainCommands : private MainCommandsContext, public CommandHandler {
 public:
  explicit MainCommands(MainCommandsContext&& context);
  ~MainCommands();

  // CommandHandler
  virtual CommandHandler* GetCommandHandler(unsigned command_id);
  virtual bool IsCommandEnabled(unsigned command_id) const;
  virtual bool IsCommandChecked(unsigned command_id) const;
  virtual void ExecuteCommand(unsigned command_id);

 private:
  promise<> ShowRenameWindowDialog();
  promise<> RenameCurrentPage();

  MainCommandContext command_context_;
};
