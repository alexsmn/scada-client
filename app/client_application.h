#pragma once

#include "app/client_application_modules.h"
#include "base/awaitable.h"
#include "base/promise.h"
#include "timed_data/timed_data_service.h"
#include "configuration/configuration_module.h"
#include "scada/data_services_factory.h"

#include <functional>
#include <memory>
#include <stack>
#include <stdexcept>

// Runs a LIFO list of teardown callbacks when destroyed. Used by
// `ClientApplication` so each module's shutdown can be registered next
// to its construction site instead of being manually ordered in the
// destructor.
class ShutdownStack {
 public:
  ~ShutdownStack() {
    while (!actions_.empty()) {
      actions_.top()();
      actions_.pop();
    }
  }
  void Push(std::function<void()> action) {
    actions_.push(std::move(action));
  }

 private:
  std::stack<std::function<void()>> actions_;
};

namespace boost {
namespace asio {
class io_context;
}
}  // namespace boost

namespace transport {
class TransportFactory;
}

class ActionManager;
class BlinkerManager;
class ConnectionStateReporter;
class ControllerRegistry;
class CoreModule;
class CreateTree;
class EventModule;
class Executor;
class FavoritesModule;
class FileSystemComponent;
class Logger;
class MainWindowManager;
class MainWindowModule;
class MasterDataServices;
class MetricService;
class NodeService;
class NodeServiceProgressTracker;
class PortfolioModule;
class Profile;
class PrintModule;
class PropertyService;
class TaskManager;
class Speech;
class WriteService;

class LoginCanceled : public std::runtime_error {
 public:
  LoginCanceled() : std::runtime_error{"Login canceled"} {}
};

struct ClientApplicationContext {
  boost::asio::io_context& io_context_;
  const std::shared_ptr<Executor> executor_;

  // TODO: Remove the `DataServicesContext` parameter.
  const std::function<promise<std::optional<DataServices>>(
      DataServicesContext&& services_context)>
      login_handler_;

  NodeServiceTreeFactory node_service_tree_factory_;

  // Optional override for testing/screenshots. If set, used instead of
  // creating a real TimedDataService from NodeService + HistoryService.
  std::unique_ptr<TimedDataService> timed_data_service_override_;

  // Optional override for testing/screenshots. If set, used instead of
  // creating a real NodeService from browse/attribute services.
  std::shared_ptr<NodeService> node_service_override_;

  ClientApplicationModuleConfigurator module_configurator_ =
      MakeDefaultClientApplicationModules();
};

class ClientApplication : private ClientApplicationContext {
 public:
  explicit ClientApplication(ClientApplicationContext&& context);
  ~ClientApplication();

  ClientApplication(const ClientApplication&) = delete;
  ClientApplication& operator=(const ClientApplication&) = delete;

  TimedDataService& timed_data_service() { return *timed_data_service_; }
  NodeService& node_service() { return *node_service_; }
  ControllerRegistry& controller_registry() { return *controller_registry_; }
  Profile& profile() { return *profile_; }
  MainWindowManager& main_window_manager();

  // Load profile and start.
  [[nodiscard]] promise<void> Start();
  // Enter the main loop.
  [[nodiscard]] promise<void> Run() { return quit_promise_; }
  [[nodiscard]] promise<void> Quit();

 private:
  struct PostLoginContext;

  void PostLogin();
  void CreateNodeService(const PostLoginContext& ctx);
  void CreateEventAndDataServices(const PostLoginContext& ctx);
  void CreateUserServices(const PostLoginContext& ctx);
  void CreateFeatureComponents(const PostLoginContext& ctx);
  void RunModuleConfigurator(const PostLoginContext& ctx);
  void CreateMainWindow(const PostLoginContext& ctx);

  ClientApplicationModuleContext BuildModuleContext(
      const PostLoginContext& ctx);

  promise<void> Login();

  // Coroutine-driven internals for the startup/login/quit flow. The public
  // `Start()`, `Login()` and `Quit()` entry points keep returning
  // `promise<void>` for compatibility with the rest of the client, but the
  // actual orchestration lives in these coroutines so the sequential
  // ordering is readable without nested `.then()` chains.
  Awaitable<void> StartAsync();
  Awaitable<void> LoginAsync();
  Awaitable<void> QuitAsync();

  void OnLoginCompleted(const DataServices& services);

  std::unique_ptr<CoreModule> core_module_;

  std::shared_ptr<Logger> logger_;
  std::unique_ptr<MetricService> metric_service_;

  std::shared_ptr<transport::TransportFactory> transport_factory_;

  std::unique_ptr<ControllerRegistry> controller_registry_;

  std::shared_ptr<MasterDataServices> master_data_services_;

  std::unique_ptr<Profile> profile_;

  std::shared_ptr<NodeService> node_service_;

  std::unique_ptr<EventModule> event_module_;
  std::unique_ptr<TimedDataService> timed_data_service_;

  std::shared_ptr<TaskManager> task_manager_;
  std::unique_ptr<PortfolioModule> portfolio_module_;
  std::unique_ptr<FavoritesModule> favorites_module_;
  std::unique_ptr<PrintModule> print_module_;
  std::unique_ptr<Speech> speech_;
  std::unique_ptr<BlinkerManager> blinker_manager_;
  std::unique_ptr<CreateTree> create_tree_;
  std::unique_ptr<PropertyService> property_service_;

  std::unique_ptr<WriteService> write_service_;

  std::unique_ptr<ConnectionStateReporter> connection_state_reporter_;

  std::unique_ptr<MainWindowModule> main_window_module_;

  std::unique_ptr<NodeServiceProgressTracker> node_service_progress_tracker_;

  std::unique_ptr<FileSystemComponent> filesystem_component_;

  std::stack<std::shared_ptr<void>> singletons_;

  bool profile_loaded_ = false;

  // Sets on `Quit` and never resets. Allows multiple `Quit` calls.
  bool quitting_ = false;
  promise<void> quit_promise_;

  // Must be the last member so it is destroyed first, running registered
  // teardown callbacks before the other members are torn down.
  ShutdownStack shutdown_stack_;
};
