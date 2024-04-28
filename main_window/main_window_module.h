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
class EventDispatcher;
class Executor;
class Favourites;
class FileCache;
class FileManager;
class LocalEvents;
class MainWindow;
class MainWindowManager;
class NodeEventProvider;
class NodeService;
class OpenedView;
class PortfolioManager;
class Profile;
class ProgressHost;
class SelectionCommands;
class SpeechService;
class TaskManager;
class TimedDataService;
struct GlobalCommandContext;
struct MainWindowContext;
struct SelectionCommandContext;

using LoginHandler = std::function<void()>;
using QuitHandler = std::function<void()>;

struct MainWindowModuleContext {
  std::shared_ptr<Executor> executor_;
  Profile& profile_;
  QuitHandler quit_handler_;
  scada::services scada_services_;
  LoginHandler login_handler_;
  TaskManager& task_manager_;
  NodeEventProvider& node_event_provider_;
  TimedDataService& timed_data_service_;
  NodeService& node_service_;
  PortfolioManager& portfolio_manager_;
  LocalEvents& local_events_;
  Favourites& favourites_;
  FileCache& file_cache_;
  FileManager& file_manager_;
  SpeechService& speech_service_;
  NodeCommandHandler node_command_handler_;
  ProgressHost& progress_host_;
  CreateTree& create_tree_;
  BasicCommandRegistry<GlobalCommandContext>& global_commands_;
  BasicCommandRegistry<SelectionCommandContext>& selection_commands_;
  // TODO: Keep either controller factory or controller registry.
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