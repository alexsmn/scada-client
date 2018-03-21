#pragma once

#include <functional>

#include "command_handler.h"

namespace events {
class EventManager;
}

namespace scada {
class SessionService;
class ViewService;
}  // namespace scada

class DialogService;
class Favourites;
class LocalEvents;
class MainWindow;
class NodeService;
class Page;
class Profile;
class Speech;
class TaskManager;

struct MainCommandsContext {
  MainWindow& main_window_;
  NodeService& node_service_;
  TaskManager& task_manager_;
  events::EventManager& event_manager_;
  LocalEvents& local_events_;
  Profile& profile_;
  scada::SessionService& session_service_;
  std::function<void()> new_main_window_;
  std::function<Page*()> find_closed_page_;
  Speech& speech_;
  Favourites& favourites_;
  DialogService& dialog_service_;
  scada::ViewService& view_service_;
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
