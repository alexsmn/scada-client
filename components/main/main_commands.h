#pragma once

#include "command_handler.h"

#include <type_traits>

namespace events {
class EventManager;
}

namespace scada {
class SessionService;
}

class DialogService;
class Favourites;
class LocalEvents;
class MainWindow;
class MainWindowManager;
class NodeService;
class Profile;
class Speech;
class TaskManager;

struct MainCommandsContext {
  MainWindow& main_window_;
  TaskManager& task_manager_;
  DialogService& dialog_service_;
  scada::SessionService& session_service_;
  events::EventManager& event_manager_;
  NodeService& node_service_;
  LocalEvents& local_events_;
  Favourites& favourites_;
  Speech& speech_;
  Profile& profile_;
  MainWindowManager& main_window_manager_;
};

class MainCommands : private MainCommandsContext, public CommandHandler {
 public:
  explicit MainCommands(MainCommandsContext&& context)
      : MainCommandsContext{std::move(context)} {}

  // CommandHandler
  virtual CommandHandler* GetCommandHandler(unsigned command_id);
  virtual bool IsCommandEnabled(unsigned command_id) const;
  virtual bool IsCommandChecked(unsigned command_id) const;
  virtual void ExecuteCommand(unsigned command_id);

 private:
  void AddToFavourites();
  void ShowRenameWindowDialog();
};
