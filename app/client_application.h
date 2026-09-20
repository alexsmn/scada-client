#pragma once

#include "base/any_executor.h"
#include "base/boost_log.h"
#include "base/lifetime.h"

#include "app/client_application_modules.h"
#include "app/login_canceled.h"
#include "base/async_completion.h"
#include "base/awaitable.h"
#include "configuration/configuration_module.h"
#include "core/default_node_command_registry.h"
#include "scada/co_result.h"
#include "scada/data_services_factory.h"
#include "scada/node_id.h"
#include "scada/status.h"
#include "services/display_selection_registry.h"
#include "services/frame_capture_registry.h"
#include "timed_data/timed_data_service.h"

#include <boost/json.hpp>

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
  void Push(std::function<void()> action) { actions_.push(std::move(action)); }

 private:
  std::stack<std::function<void()>> actions_;
};

namespace transport {
class TransportFactory;
}

class ActionManager;
class AppNapSuppressor;
class BlinkerManager;
class ConnectionStateReporter;
class ControllerRegistry;
class CoreModule;
class CreateTree;
class EventModule;
class NodeEventProvider;
class Favourites;
class LocalEvents;
class AdministrationModule;
class FavoritesModule;
class FileSystemComponent;
class MainWindowManager;
class MainWindowModule;
class MasterDataServices;
class NodeService;
class NodeServiceProgressTracker;
class OpenedViewCommandRegistry;
class PortfolioModule;
class Profile;
class PrintModule;
class PropertyService;
class TaskManager;
class Speech;
class UiCommandRegistry;
class WriteService;

namespace scada::metrics {
class OpenTelemetryMetrics;
}

struct ClientApplicationContext {
  const AnyExecutor executor_;

  // TODO: Remove the `DataServicesContext` parameter.
  const std::function<Awaitable<std::optional<DataServices>>(
      DataServicesContext&& services_context)>
      login_handler_;

  NodeServiceTreeFactory node_service_tree_factory_;

  // Optional override for testing/screenshots. If set, used instead of
  // creating a real TimedDataService from NodeService + HistoryService.
  std::unique_ptr<TimedDataService> timed_data_service_override_;

  // Optional override for testing/screenshots. If set, used instead of
  // creating a real NodeService from browse/attribute services.
  std::shared_ptr<NodeService> node_service_override_;

  DefaultNodeCommandRegistry default_node_commands_;

  ClientApplicationModuleConfigurator module_configurator_ =
      MakeDefaultClientApplicationModules();
};

class ClientApplication : private ClientApplicationContext {
 public:
  explicit ClientApplication(ClientApplicationContext&& context);
  ~ClientApplication();

  ClientApplication(const ClientApplication&) = delete;
  ClientApplication& operator=(const ClientApplication&) = delete;

  TimedDataService& timed_data_service() SCADA_LIFETIME_BOUND {
    return *timed_data_service_;
  }
  NodeService& node_service() SCADA_LIFETIME_BOUND { return *node_service_; }
  // The live event source, for surfaces that show the actionable backlog
  // outside a view (e.g. the display frame's Recent-events strip).
  NodeEventProvider& node_event_provider() SCADA_LIFETIME_BOUND;
  // The saved-window store behind the Favorites pane. Exposed so the
  // screenshot generator can seed it: the pane renders whatever the profile
  // holds, and with an empty profile it captures an empty panel.
  Favourites& favourites() SCADA_LIFETIME_BOUND;
  // Client-side events (connection established/lost, and the like). Exposed
  // for the same reason as favourites(): nothing reports one in an offline
  // fixture, so the journal's «Local Event» rows — the ones the manual's
  // event-journal pages are built around — never appear unless seeded.
  LocalEvents& local_events() SCADA_LIFETIME_BOUND;
  ControllerRegistry& controller_registry() SCADA_LIFETIME_BOUND {
    return *controller_registry_;
  }
  Profile& profile() SCADA_LIFETIME_BOUND { return *profile_; }
  MainWindowManager& main_window_manager() SCADA_LIFETIME_BOUND;
  bool HasSelectionCommandForTesting(unsigned command_id) const;
  bool HasGlobalCommandForTesting(unsigned command_id) const;

  // Saves the current profile to the logged-in server user, or to
  // `target_user_id` when provided.
  //
  // The Qt document is wrapped into the envelope last read from the server, so
  // the sections this client does not own -- the web client's, above all --
  // survive. See `profile/profile_envelope.h`.
  [[nodiscard]] scada::CoStatus SaveProfileToServer(
      scada::NodeId target_user_id = {});

  // Load profile and start.
  [[nodiscard]] Awaitable<void> Start();
  // Enter the main loop.
  [[nodiscard]] Awaitable<void> Run();
  [[nodiscard]] Awaitable<void> Quit();

 private:
  struct PostLoginContext;

  // Awaitable because the profile is read from the server before anything is
  // built on it: `CreateMainWindow` at the end of this phase lays out whatever
  // the profile holds, so a read landing after it would be a read landing too
  // late.
  [[nodiscard]] Awaitable<void> PostLoginAsync();

  // Creates `profile_` and fills it from exactly ONE source -- the server's
  // copy when it carries a Qt section, otherwise the local file.
  //
  // One source and not two on purpose: `Profile::Load` overlays rather than
  // replaces, so calling it twice accumulates pages and windows instead of
  // superseding them.
  [[nodiscard]] Awaitable<void> LoadProfileAsync();

  // The profile envelope stored on the server for the signed-in user, or a
  // null value when there is none, the session is anonymous, or the read
  // fails. Never throws: a server that cannot answer means the local file, not
  // a failed startup.
  [[nodiscard]] Awaitable<boost::json::value> ReadServerProfileAsync();

  // Writes the profile to the server, reporting a failure rather than
  // swallowing it. Called from `QuitAsync` -- see the note there for why not
  // from the destructor.
  [[nodiscard]] Awaitable<void> SaveProfileToServerOnQuitAsync();
  void CreateNodeService(const PostLoginContext& ctx);
  void CreateEventAndDataServices(const PostLoginContext& ctx);
  void CreateUserServices(const PostLoginContext& ctx);
  void CreateFeatureComponents(const PostLoginContext& ctx);
  void RunModuleConfigurator(const PostLoginContext& ctx);
  void CreateMainWindow(const PostLoginContext& ctx);

  ClientApplicationModuleContext BuildModuleContext(
      const PostLoginContext& ctx);

  Awaitable<void> StartAsync();
  Awaitable<void> Login();
  Awaitable<void> LoginAsync();
  Awaitable<void> RunAsync();
  Awaitable<void> QuitAsync();

  void OnLoginCompleted(const DataServices& services);

  std::unique_ptr<CoreModule> core_module_;

  std::shared_ptr<BoostLogger> logger_;
  std::unique_ptr<scada::metrics::OpenTelemetryMetrics> metrics_runtime_;

  std::shared_ptr<transport::TransportFactory> transport_factory_;

  std::unique_ptr<ControllerRegistry> controller_registry_;
  std::unique_ptr<UiCommandRegistry> ui_command_registry_;
  std::unique_ptr<OpenedViewCommandRegistry> opened_view_command_registry_;

  std::shared_ptr<MasterDataServices> master_data_services_;

  std::unique_ptr<Profile> profile_;

  std::shared_ptr<NodeService> node_service_;

  std::unique_ptr<EventModule> event_module_;
  std::unique_ptr<TimedDataService> timed_data_service_;

  std::shared_ptr<TaskManager> task_manager_;
  std::unique_ptr<PortfolioModule> portfolio_module_;
  std::unique_ptr<FavoritesModule> favorites_module_;
  std::unique_ptr<AdministrationModule> administration_module_;
  std::unique_ptr<PrintModule> print_module_;
  std::unique_ptr<Speech> speech_;
  std::unique_ptr<BlinkerManager> blinker_manager_;
  // Which devices this session has armed for frame capture. Plain member, not
  // a unique_ptr: it is pure state with no dependencies, and it must outlive
  // both the controllers that arm and the status strip that reports.
  FrameCaptureRegistry frame_capture_registry_;
  DisplaySelectionRegistry display_selection_registry_;

  std::unique_ptr<CreateTree> create_tree_;
  std::unique_ptr<PropertyService> property_service_;

  std::unique_ptr<WriteService> write_service_;

  std::unique_ptr<ConnectionStateReporter> connection_state_reporter_;

  // Keeps macOS App Nap from coalescing the Qt event loop that dispatches every
  // completion handler (services/app_nap_suppressor.h). Held for the logged-in
  // lifetime; a no-op on other platforms.
  std::unique_ptr<AppNapSuppressor> app_nap_suppressor_;

  std::unique_ptr<MainWindowModule> main_window_module_;

  std::unique_ptr<NodeServiceProgressTracker> node_service_progress_tracker_;

  std::unique_ptr<FileSystemComponent> filesystem_component_;

  std::stack<std::shared_ptr<void>> singletons_;

  bool profile_loaded_ = false;
  scada::UInt64 profile_revision_ = 0;

  // The envelope last read from the server, so a save can put the Qt section
  // back into it without disturbing the web client's. Null when nothing was
  // read -- `profile_envelope::Wrap` takes that to mean "there was none" and
  // builds a fresh envelope.
  boost::json::value server_profile_envelope_;

  // Sets on `Quit` and never resets. Allows multiple `Quit` calls.
  bool quitting_ = false;
  scada::base::AsyncCompletion quit_completion_;

  // Must be the last member so it is destroyed first, running registered
  // teardown callbacks before the other members are torn down.
  ShutdownStack shutdown_stack_;
};
