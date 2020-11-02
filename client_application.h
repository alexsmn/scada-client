#pragma once

#include "common/aliases.h"
#include "components/main/main_window_context.h"
#include "core/data_services_factory.h"
#include "core/session_state_observer.h"

#include <memory>

namespace base {
class Timer;
}

namespace boost {
namespace asio {
class io_context;
}
}  // namespace boost

namespace net {
class TransportFactory;
}

class ActionManager;
class ConnectionStateReporter;
class EventFetcher;
class EventNotifier;
class Executor;
class Favourites;
class FileCache;
class FileSynchronizer;
class Logger;
class LocalEvents;
class MainWindowManager;
class MasterDataServices;
class ModusModule2;
class NodeService;
class PortfolioManager;
class Profile;
class TaskManager;
class TimedDataService;
class Speech;

struct ClientApplicationContext {
  std::function<std::unique_ptr<MainWindow>(MainWindowContext&& context)>
      main_window_factory_;
  std::function<void()> quit_handler_;
};

class ClientApplication : private ClientApplicationContext,
                          private scada::SessionStateObserver {
 public:
  explicit ClientApplication(ClientApplicationContext&& context);
  ~ClientApplication();

  ClientApplication(const ClientApplication&) = delete;
  ClientApplication& operator=(const ClientApplication&) = delete;

  bool Login();

  // Called after Login().
  void Start();

 private:
  std::shared_ptr<NodeService> CreateRemoteNodeService();
  std::shared_ptr<NodeService> CreateAddressSpaceNodeService();

  MainWindowContext MakeMainWindowContext(int window_id);

  void OnEvents(bool has_events);

  void Quit();

  // scada::SessionStateObserver
  virtual void OnSessionCreated() override;
  virtual void OnSessionDeleted(const scada::Status& status) override;

  std::shared_ptr<Logger> logger_;

  std::unique_ptr<boost::asio::io_context> io_context_;
  std::shared_ptr<Executor> executor_;
  std::unique_ptr<base::Timer> io_context_timer_;
  std::unique_ptr<net::TransportFactory> transport_factory_;

  std::shared_ptr<MasterDataServices> master_data_services_;

  std::unique_ptr<Profile> profile_;

  std::shared_ptr<NodeService> node_service_;

  std::unique_ptr<EventFetcher> event_fetcher_;
  AliasResolver alias_resolver_;
  std::unique_ptr<FileSynchronizer> file_synchronizer_;
  std::unique_ptr<TimedDataService> timed_data_service_;

  std::unique_ptr<LocalEvents> local_events_;
  std::unique_ptr<EventNotifier> event_notifier_;
  std::unique_ptr<TaskManager> task_manager_;
  std::unique_ptr<ActionManager> action_manager_;
  std::unique_ptr<PortfolioManager> portfolio_manager_;
  std::unique_ptr<Favourites> favourites_;
  std::unique_ptr<Speech> speech_;

  std::unique_ptr<FileCache> file_cache_;

  std::unique_ptr<ModusModule2> modus_module_;

  std::unique_ptr<ConnectionStateReporter> connection_state_reporter_;

  std::unique_ptr<MainWindowManager> main_window_manager_;

  bool profile_loaded_ = false;
};
