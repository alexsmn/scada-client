#pragma once

#include "base/promise.h"
#include "common/aliases.h"
#include "main_window/main_window_context.h"
#include "scada/data_services_factory.h"

#include <memory>

namespace boost {
namespace asio {
class io_context;
}
}  // namespace boost

namespace net {
class TransportFactory;
}

class ActionManager;
class BlinkerManager;
class ConnectionStateReporter;
class ControllerRegistry;
class CreateTree;
class EventDispatcher;
class EventFetcher;
class Executor;
class Favourites;
class FileCache;
class FileRegistry;
class FileSystemComponent;
class Logger;
class LocalEvents;
class MainWindowManager;
class MasterDataServices;
class NodeService;
class NodeServiceProgressTracker;
class PortfolioManager;
class Profile;
class ProgressHost;
class PropertyService;
class TaskManager;
class TimedDataService;
class Speech;
class WriteService;

#if !defined(UI_WT)
class ModusModule2;
#endif

struct ClientApplicationContext {
  boost::asio::io_context& io_context_;
  const std::shared_ptr<Executor> executor_;
  const std::function<std::unique_ptr<MainWindow>(MainWindowContext&& context)>
      main_window_factory_;
  const std::function<promise<std::optional<DataServices>>(
      DataServicesContext&& services_context)>
      login_handler_;
  const std::function<void()> quit_handler_;
};

class ClientApplication : private ClientApplicationContext {
 public:
  explicit ClientApplication(ClientApplicationContext&& context);
  ~ClientApplication();

  ClientApplication(const ClientApplication&) = delete;
  ClientApplication& operator=(const ClientApplication&) = delete;

  void Start();

 private:
  MainWindowContext MakeMainWindowContext(int window_id);

  promise<bool> Login();
  void OnLoginCompleted(DataServices services);
  void OnStartLoginCompleted();

  void OnEvents(bool has_events);

  void Quit();

  std::shared_ptr<Logger> logger_;

  std::unique_ptr<net::TransportFactory> transport_factory_;

  std::unique_ptr<ControllerRegistry> controller_registry_;

  std::vector<std::shared_ptr<void>> singletons_;

  std::shared_ptr<MasterDataServices> master_data_services_;

  std::unique_ptr<Profile> profile_;

  std::shared_ptr<NodeService> node_service_;

  std::shared_ptr<EventFetcher> event_fetcher_;
  AliasResolver alias_resolver_;
  std::unique_ptr<FileSystemComponent> filesystem_component_;
  std::unique_ptr<TimedDataService> timed_data_service_;

  std::unique_ptr<LocalEvents> local_events_;
  std::unique_ptr<ProgressHost> progress_host_;
  std::shared_ptr<TaskManager> task_manager_;
  std::unique_ptr<ActionManager> action_manager_;
  std::unique_ptr<PortfolioManager> portfolio_manager_;
  std::unique_ptr<Favourites> favourites_;
  std::unique_ptr<Speech> speech_;
  std::unique_ptr<BlinkerManager> blinker_manager_;
  std::unique_ptr<CreateTree> create_tree_;
  std::unique_ptr<PropertyService> property_service_;

  std::unique_ptr<FileRegistry> file_registry_;
  std::unique_ptr<FileCache> file_cache_;

  std::unique_ptr<WriteService> write_service_;

#if !defined(UI_WT)
  std::unique_ptr<ModusModule2> modus_module_;
#endif

  std::unique_ptr<ConnectionStateReporter> connection_state_reporter_;

  std::unique_ptr<MainWindowManager> main_window_manager_;

  std::unique_ptr<EventDispatcher> event_dispatcher_;

  std::unique_ptr<NodeServiceProgressTracker> node_service_progress_tracker_;

  bool profile_loaded_ = false;
};
