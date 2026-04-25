#pragma once

#include "base/awaitable.h"
#include "base/promise.h"
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
class Executor;
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
  std::shared_ptr<Executor> executor_;
  MainWindowInterface& main_window_;
  TaskManager& task_manager_;
  DialogService& dialog_service_;
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

class MainWindowCommands : private MainWindowCommandsContext,
                           public CommandHandler {
 public:
  explicit MainWindowCommands(MainWindowCommandsContext&& context);
  ~MainWindowCommands();

  // CommandHandler
  virtual CommandHandler* GetCommandHandler(unsigned command_id);
  virtual bool IsCommandEnabled(unsigned command_id) const;
  virtual bool IsCommandChecked(unsigned command_id) const;
  virtual void ExecuteCommand(unsigned command_id);

 private:
  promise<> ShowRenameWindowDialog();
  Awaitable<void> ShowRenameWindowDialogAsync();
  promise<> RenameCurrentPage();
  Awaitable<void> RenameCurrentPageAsync();

  GlobalCommandContext command_context_;
};
