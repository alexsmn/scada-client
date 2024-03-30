#pragma once

#include "controller/controller_factory.h"
#include "core/node_command_context.h"

#include <functional>
#include <memory>
#include <stack>

template <class T>
class BasicCommandRegistry;

class ActionManager;
class CreateTree;
class EventFetcher;
class EventDispatcher;
class Executor;
class Favourites;
class FileCache;
class FileManager;
class LocalEvents;
class MainWindow;
class MainWindowManager;
class MasterDataServices;
class NodeService;
class OpenedView;
class PortfolioManager;
class Profile;
class ProgressHost;
class SelectionCommands;
class Speech;
class TaskManager;
class TimedDataService;
struct GlobalCommandContext;
struct MainWindowContext;
struct SelectionCommandContext;

using MainWindowFactory =
    std::function<std::unique_ptr<MainWindow>(MainWindowContext&& context)>;

using LoginHandler = std::function<void()>;
using QuitHandler = std::function<void()>;

struct MainWindowModuleContext {
  std::shared_ptr<Executor> executor_;
  Profile& profile_;
  QuitHandler quit_handler_;
  MasterDataServices& master_data_services_;
  LoginHandler login_handler_;
  TaskManager& task_manager_;
  EventFetcher& event_fetcher_;
  TimedDataService& timed_data_service_;
  NodeService& node_service_;
  PortfolioManager& portfolio_manager_;
  LocalEvents& local_events_;
  Favourites& favourites_;
  FileCache& file_cache_;
  FileManager& file_manager_;
  Speech& speech_;
  const NodeCommandHandler node_command_handler_;
  ProgressHost& progress_host_;
  CreateTree& create_tree_;
  BasicCommandRegistry<GlobalCommandContext>& global_commands_;
  BasicCommandRegistry<SelectionCommandContext>& selection_commands_;
  ControllerFactory controller_factory_;
};

class MainWindowModule : private MainWindowModuleContext {
 public:
  explicit MainWindowModule(MainWindowModuleContext&& context);
  ~MainWindowModule();

  MainWindowManager& main_window_manager() { return *main_window_manager_; }

 private:
  MainWindowContext MakeMainWindowContext(int window_id);

  std::unique_ptr<OpenedView> CreateOpenedView(MainWindow& main_window,
                                               WindowDefinition& window_def);

  void OnEvents(bool has_events);

  std::unique_ptr<ActionManager> action_manager_;
  std::unique_ptr<MainWindowManager> main_window_manager_;
  std::unique_ptr<EventDispatcher> event_dispatcher_;
  std::shared_ptr<SelectionCommands> selection_commands_object_;

  std::stack<std::shared_ptr<void>> singletons_;
};