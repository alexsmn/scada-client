#pragma once

#include "base/any_executor.h"

#include "core/global_command_context.h"

#include <functional>
#include <memory>
#include <type_traits>

namespace scada {
class SessionService;
}

class ActionManager;
class DialogService;
class Favourites;
class LocalEvents;
class MainWindowInterface;
class MainWindowManager;
class NodeEventProvider;
class NodeService;
class Profile;
class SpeechService;
class TaskManager;
struct GlobalCommandContext;

struct MainWindowCommandsContext {
  AnyExecutor executor_;
  TaskManager& task_manager_;
  scada::SessionService& session_service_;
  NodeEventProvider& node_event_provider_;
  NodeService& node_service_;
  LocalEvents& local_events_;
  Favourites& favourites_;
  SpeechService& speech_service_;
  Profile& profile_;
  MainWindowManager& main_window_manager_;
  std::function<void(bool login)> login_handler_;
  ActionManager& action_manager_;
};

class MainWindowCommands : private MainWindowCommandsContext {
 public:
  explicit MainWindowCommands(MainWindowCommandsContext&& context);
  ~MainWindowCommands();
};

struct MainWindowCommandHandlerContext {
  AnyExecutor executor_;
  MainWindowInterface& main_window_;
  DialogService& dialog_service_;
  scada::SessionService& session_service_;
  ActionManager& action_manager_;
};

struct Action;

class MainWindowCommandHandler : private MainWindowCommandHandlerContext {
 public:
  explicit MainWindowCommandHandler(MainWindowCommandHandlerContext&& context);
  ~MainWindowCommandHandler();

  Action* FindAction(unsigned command_id);
  bool IsActionEnabled(unsigned command_id) const;
  bool IsActionChecked(unsigned command_id) const;
  void ExecuteAction(unsigned command_id);

 private:
  GlobalCommandContext command_context_;
};
