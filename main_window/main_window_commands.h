#pragma once

#include "base/any_executor.h"

#include "controller/command_handler.h"
#include "core/global_command_context.h"

#include <functional>
#include <memory>
#include <type_traits>

namespace scada {
class SessionService;
}

template <class T>
class BasicCommandRegistry;

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
  BasicCommandRegistry<GlobalCommandContext>& global_commands_;
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
  BasicCommandRegistry<GlobalCommandContext>& global_commands_;
};

class MainWindowCommandHandler : private MainWindowCommandHandlerContext,
                                 public CommandHandler {
 public:
  explicit MainWindowCommandHandler(MainWindowCommandHandlerContext&& context);
  ~MainWindowCommandHandler();

  // CommandHandler
  virtual CommandHandler* GetCommandHandler(unsigned command_id);
  virtual bool IsCommandEnabled(unsigned command_id) const;
  virtual bool IsCommandChecked(unsigned command_id) const;
  virtual void ExecuteCommand(unsigned command_id);

 private:
  GlobalCommandContext command_context_;
};
