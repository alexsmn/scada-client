#pragma once

#include "common/aliases.h"
#include "controller/command_registry.h"
#include "main_window/main_command_context.h"
#include "main_window/main_window_context.h"

#include <functional>
#include <memory>

class ActionManager;
class BlinkerManager;
class CreateTree;
class EventFetcher;
class EventDispatcher;
class Executor;
class Favourites;
class FileCache;
class FileRegistry;
class LocalEvents;
class MainWindow;
class MainWindowManager;
class MasterDataServices;
class NodeService;
class PortfolioManager;
class Profile;
class ProgressHost;
class PropertyService;
class Speech;
class TaskManager;
class TimedDataService;
struct MainWindowContext;

using MainWindowFactory =
    std::function<std::unique_ptr<MainWindow>(MainWindowContext&& context)>;

using LoginHandler = std::function<void()>;

using QuitHandler = std::function<void()>;

struct MainWindowModuleContext {
  std::shared_ptr<Executor> executor_;
  Profile& profile_;
  MainWindowFactory main_window_factory_;
  QuitHandler quit_handler_;
  MasterDataServices& master_data_services_;
  AliasResolver alias_resolver_;
  LoginHandler login_handler_;
  TaskManager& task_manager_;
  EventFetcher& event_fetcher_;
  TimedDataService& timed_data_service_;
  NodeService& node_service_;
  PortfolioManager& portfolio_manager_;
  LocalEvents& local_events_;
  Favourites& favourites_;
  FileCache& file_cache_;
  BlinkerManager& blinker_manager_;
  Speech& speech_;
  FileRegistry& file_registry_;
  ProgressHost& progress_host_;
  PropertyService& property_service_;
  CreateTree& create_tree_;
};

class MainWindowModule : private MainWindowModuleContext {
 public:
  explicit MainWindowModule(MainWindowModuleContext&& context);
  ~MainWindowModule();

  BasicCommandRegistry<MainCommandContext>& commands() { return commands_; }

 private:
  MainWindowContext MakeMainWindowContext(int window_id);

  void OnEvents(bool has_events);

  BasicCommandRegistry<MainCommandContext> commands_;

  std::unique_ptr<ActionManager> action_manager_;
  std::unique_ptr<MainWindowManager> main_window_manager_;
  std::unique_ptr<EventDispatcher> event_dispatcher_;
};