#include "app/client_application.h"

#include "administration/administration_module.h"
#include "aui/translation.h"
#include "base/any_executor.h"
#include "base/blinker.h"
#include "base/boost_log.h"
#include "base/program_options.h"
#include "common/audit.h"
#include "common/master_data_services.h"
#include "configuration/tree/node_service_tree_impl.h"
#include "controller/command_ui_registry.h"
#include "controller/controller_factory_impl.h"
#include "controller/controller_registry.h"
#include "core/core_module.h"
#include "events/event_module.h"
#include "events/local_events.h"
#include "export/configuration/export_configuration_module.h"
#include "export/csv/csv_export_module.h"
#include "export/excel/excel_export_module.h"
#include "favorites/favorites_module.h"
#include "filesystem/filesystem_component.h"
#include "main_window/main_window_module.h"
#include "main_window/main_window_util.h"
#include "main_window/opened_view/opened_view_command_registry.h"
#include "metrics/otel_metrics.h"
#include "model/node_id_util.h"
#include "model/security_node_ids.h"
#include "modules/limits/limits_module.h"
#include "modules/node_service_progress_tracker/node_service_progress_tracker.h"
#include "modules/write/write_module.h"
#include "modules/write/write_service_impl.h"
#include "node_service/node_service_factory.h"
#include "portfolio/portfolio_module.h"
#include "print/service/print_module.h"
#include "profile/profile.h"
#include "profile/profile_envelope.h"
#include "project.h"
#include "properties/property_service.h"
#include "remote/remote_services.h"
#include "resources/common_resources.h"
#include "scada/co_result.h"
#include "scada/service_context.h"
#include "services/alias_resolver_factory.h"
#include "services/app_nap_suppressor.h"
#include "services/connection_state_reporter.h"
#include "services/create_tree.h"
#include "services/speech_service_impl.h"
#include "services/task_manager_impl.h"
#include "timed_data/timed_data_service_factory.h"

#include <boost/json.hpp>

#include <transport/transport_factory_impl.h>

// Windows.h #defines ReportEvent to ReportEventA/W. Undo it.
#ifdef ReportEvent
#undef ReportEvent
#endif

using namespace std::chrono_literals;

namespace {

scada::ServiceLogParams ReadServiceLogParamsFromCommandLine() {
  return {.log_read = client::HasOption("log-service-read"),
          .log_browse = client::HasOption("log-service-browse"),
          .log_history = client::HasOption("log-service-history"),
          .log_event = client::HasOption("log-service-event"),
          .log_model_change_event =
              client::HasOption("log-service-model-change-event"),
          .log_node_semantics_change_event =
              client::HasOption("log-service-node-semantics-change-event")};
}

// OTLP/gRPC collector the process metrics are exported to, from
// `--otlp-endpoint=<host:port>`. Empty (the default) means no export: an empty
// endpoint makes the OTLP gRPC exporter log "empty endpoint" and hand back a
// null channel, so nothing leaves the process. Point it at a local viewer
// (otel-desktop-viewer, an OTel Collector, Jaeger) to see client metrics.
std::string ReadOtlpEndpointFromCommandLine() {
  return client::GetOptionValue("otlp-endpoint");
}

// Metric export period, from `--otlp-export-interval-ms`. The default suits a
// long-running desktop session, but it outlives short-lived runs entirely — an
// E2E case lasts well under a minute, so without shortening this the client
// exits before its first export and reports nothing. Malformed values fall
// back to the default rather than failing startup: telemetry cadence must
// never keep the client from running.
std::chrono::milliseconds ReadOtlpExportIntervalFromCommandLine() {
  constexpr std::chrono::milliseconds kDefaultInterval = 1min;
  const std::string value = client::GetOptionValue("otlp-export-interval-ms");
  if (value.empty())
    return kDefaultInterval;
  try {
    const long long parsed = std::stoll(value);
    if (parsed <= 0)
      return kDefaultInterval;
    return std::chrono::milliseconds{parsed};
  } catch (const std::exception&) {
    return kDefaultInterval;
  }
}

}  // namespace

REGISTER_DATA_SERVICES("Scada",
                       u"Telecontrol",
                       CreateRemoteServices,
                       "localhost");

#if CLIENT_HAS_VIDICON_SERVICES
extern bool CreateVidiconServices(const DataServicesContext& context,
                                  DataServices& services);

REGISTER_DATA_SERVICES("Vidicon",
                       u"Vidicon",
                       CreateVidiconServices,
                       "localhost");
#endif

// Locals shared across the PostLogin() phase helpers.
struct ClientApplication::PostLoginContext {
  scada::services audited_scada_services;
  scada::client scada_client;
  std::shared_ptr<scada::HistoryService> coroutine_history_service;
  AliasResolver alias_resolver;
};

ClientApplication::ClientApplication(ClientApplicationContext&& context)
    : ClientApplicationContext{std::move(context)},
      metrics_runtime_{std::make_unique<scada::metrics::OpenTelemetryMetrics>(
          scada::metrics::OpenTelemetryMetricsOptions{
              .service_name = "scada-client",
              .export_interval = ReadOtlpExportIntervalFromCommandLine(),
              .endpoint = ReadOtlpEndpointFromCommandLine()})},
      controller_registry_{std::make_unique<ControllerRegistry>()},
      ui_command_registry_{std::make_unique<UiCommandRegistry>()},
      opened_view_command_registry_{
          std::make_unique<OpenedViewCommandRegistry>()},
      master_data_services_{std::make_shared<MasterDataServices>(executor_)},
      quit_completion_{executor_} {
  logger_ = std::make_shared<BoostLogger>(LOG_NAME("client"));

  transport_factory_ = transport::CreateTransportFactory();

  core_module_ = std::make_unique<CoreModule>(executor_);

  shutdown_stack_.Push([this] { transport_factory_.reset(); });
  shutdown_stack_.Push([this] { core_module_.reset(); });
  shutdown_stack_.Push([this] { master_data_services_ = nullptr; });
}

ClientApplication::~ClientApplication() {
  // Save profile before shutdown_stack_ runs teardown callbacks (which
  // include resetting profile_ itself).
  if (profile_ && profile_loaded_) {
    profile_->Save();
  }
  // shutdown_stack_ is the last declared member: its destructor runs
  // registered teardown callbacks LIFO, then the remaining members
  // auto-destroy in reverse declaration order.
}

MainWindowManager& ClientApplication::main_window_manager() {
  return main_window_module_->main_window_manager();
}

NodeEventProvider& ClientApplication::node_event_provider() {
  return event_module_->node_event_provider();
}

Favourites& ClientApplication::favourites() {
  return favorites_module_->favourites();
}

LocalEvents& ClientApplication::local_events() {
  return event_module_->local_events();
}

bool ClientApplication::HasSelectionCommandForTesting(
    unsigned command_id) const {
  return core_module_ &&
         core_module_->selection_commands().FindCommand(command_id) != nullptr;
}

bool ClientApplication::HasGlobalCommandForTesting(unsigned command_id) const {
  return core_module_ &&
         core_module_->global_commands().FindCommand(command_id) != nullptr;
}

namespace {

// The nested property nodes the server publishes beside a user node, and which
// the web client reads and writes under the same names.
constexpr std::string_view kProfileJsonProperty = "ProfileJson";
constexpr std::string_view kProfileRevisionProperty = "ProfileRevision";

}  // namespace

Awaitable<void> ClientApplication::LoadProfileAsync() {
  profile_ = std::make_unique<Profile>();
  profile_loaded_ = true;
  shutdown_stack_.Push([this] { profile_.reset(); });

  const boost::json::value server = co_await ReadServerProfileAsync();

  // **The server wins only when it actually holds a Qt profile.** A document
  // written by the web client alone carries a `qt.profile` the web filled in;
  // one that has never been written carries nothing. Taking an empty section
  // over the local file would discard this machine's window layout on the
  // first server-backed sign-in, and the first save would then push that
  // emptiness back -- so an empty section means "fall back", and the save on
  // quit populates it from the file's content instead.
  const boost::json::value qt_section = profile_envelope::Unwrap(server);
  const bool server_has_profile =
      qt_section.is_object() && !qt_section.as_object().empty();

  if (server_has_profile) {
    server_profile_envelope_ = server;
    profile_->Load(server);
    LOG_INFO(*logger_) << "Profile loaded from server";
  } else {
    // Remembered even when empty: it is what the save must put the Qt section
    // back into, and it may already carry the web client's section.
    if (server.is_object())
      server_profile_envelope_ = server;
    profile_->Load();
  }
  co_return;
}

Awaitable<boost::json::value> ClientApplication::ReadServerProfileAsync() {
  if (!master_data_services_)
    co_return boost::json::value{};

  auto services = master_data_services_->as_services();
  if (!services.session_service || !services.attribute_service)
    co_return boost::json::value{};

  // Anonymous has no server identity to hang a profile on, so there is nothing
  // to read -- the same conclusion the web client reaches, by the same route.
  const scada::NodeId user_id = services.session_service->GetUserId();
  if (user_id.is_null())
    co_return boost::json::value{};

  auto results = co_await services.attribute_service->Read(
      scada::ServiceContext{},
      {scada::ReadValueId{
           .node_id = MakeNestedNodeId(user_id, kProfileJsonProperty),
           .attribute_id = scada::AttributeId::Value},
       scada::ReadValueId{
           .node_id = MakeNestedNodeId(user_id, kProfileRevisionProperty),
           .attribute_id = scada::AttributeId::Value}});

  // **A server that cannot answer means the local file, never a failed
  // startup.** A deployment whose users carry no profile properties at all is
  // a supported one -- it is what every build before this change talked to.
  //
  // Measured 2026-09-12 against the E2E single tier: this read answers **Good
  // with an empty value** even when the config DB demonstrably holds a
  // profile, because `SaveProfile` writes the DB without refreshing the
  // published property node, so the address space keeps the value it was built
  // with for the life of the server process. That is a server-side gap and the
  // same shape as backlog 686's unresolved half -- the client writes somewhere
  // the server accepts and the server does not make the write visible on read.
  // Until it closes, this path is correct and finds nothing, and the local file
  // is what actually loads.
  if (!results.ok() || results->size() < 2) {
    LOG_INFO(*logger_) << "No server profile; using the local file";
    co_return boost::json::value{};
  }

  const scada::String* json = (*results)[0].value.get_if<scada::String>();
  if (const scada::UInt64* revision =
          (*results)[1].value.get_if<scada::UInt64>()) {
    profile_revision_ = *revision;
  }
  if (!json || json->empty())
    co_return boost::json::value{};

  boost::system::error_code ec;
  boost::json::value parsed = boost::json::parse(*json, ec);
  if (ec) {
    LOG_ERROR(*logger_) << "Server profile is not valid JSON: " << ec.message();
    co_return boost::json::value{};
  }
  co_return parsed;
}

Awaitable<void> ClientApplication::SaveProfileToServerOnQuitAsync() {
  if (!profile_ || !profile_loaded_)
    co_return;

  auto services = master_data_services_ ? master_data_services_->as_services()
                                        : scada::services{};
  if (!services.session_service ||
      services.session_service->GetUserId().is_null()) {
    co_return;  // Anonymous, or no session: the local file is the whole of it.
  }

  const scada::Status status = co_await SaveProfileToServer();
  if (scada::IsGood(status.code()))
    co_return;

  // **Reported, not swallowed.** A save that can fail silently is worse than
  // no save -- the operator would believe their layout was kept. The local
  // file is still written from ~ClientApplication, so nothing is lost on THIS
  // machine; what the failure costs is the copy that follows them to another.
  LOG_ERROR(*logger_) << "Profile save to server failed: "
                      << static_cast<unsigned>(status.code());
  if (event_module_) {
    // TODO: Localize.
    event_module_->local_events().ReportEvent(
        LocalEvents::SEV_ERROR,
        Translate("Failed to save the profile to the server; it is kept on "
                  "this computer only"));
  }
  co_return;
}

scada::CoStatus ClientApplication::SaveProfileToServer(
    scada::NodeId target_user_id) {
  if (!profile_ || !master_data_services_) {
    co_return scada::StatusCode::Bad;
  }

  auto services = master_data_services_->as_services();
  if (!services.session_service || !services.method_service) {
    co_return scada::StatusCode::Bad;
  }

  auto user_id = services.session_service->GetUserId();
  if (!target_user_id.is_null()) {
    user_id = std::move(target_user_id);
  }
  // Wrapped, never the flat document: the variable is shared with the web
  // client, and writing this client's own shape over it silently destroys the
  // other's settings -- there is no error, the next read simply comes back
  // without them. `Wrap` replaces `qt.profile` and preserves everything else
  // in the envelope this session read at login.
  auto profile_json = boost::json::serialize(profile_envelope::Wrap(
      profile_->SaveToValue(), server_profile_envelope_));
  // SaveProfile returns no output arguments; only its status matters here.
  auto status =
      (co_await services.method_service->Call(
           user_id, scada::security::id::UserType_SaveProfile,
           {scada::String{std::move(profile_json)}, profile_revision_},
           scada::ServiceContext{}))
          .status();
  if (scada::IsGood(status.code())) {
    ++profile_revision_;
  }
  co_return status;
}

Awaitable<void> ClientApplication::Start() {
  return StartAsync();
}

Awaitable<void> ClientApplication::StartAsync() {
  RegisterClientApplicationModules();
  co_await LoginAsync();
  co_await PostLoginAsync();
  co_return;
}

Awaitable<void> ClientApplication::PostLoginAsync() {
  PostLoginContext ctx{
      .audited_scada_services = master_data_services_->as_services(),
      .scada_client = scada::client{master_data_services_->as_services()},
      .coroutine_history_service =
          std::static_pointer_cast<scada::HistoryService>(
              master_data_services_),
      .alias_resolver = {}};

  CreateNodeService(ctx);
  ctx.alias_resolver = CreateAliasResolver(*node_service_);

  // Before anything reads the profile. `CreateEventAndDataServices` hands
  // `*profile_` to EventModule by reference and `CreateMainWindow` lays out
  // whatever it holds, so this is the last point at which the profile can
  // still be replaced wholesale.
  co_await LoadProfileAsync();

  singletons_.emplace(std::make_shared<CsvExportModule>(CsvExportModuleContext{
      .ui_command_registry_ = *ui_command_registry_,
      .opened_view_commands_ = *opened_view_command_registry_}));
  singletons_.emplace(
      std::make_shared<ExcelExportModule>(ExcelExportModuleContext{
          .ui_command_registry_ = *ui_command_registry_,
          .opened_view_commands_ = *opened_view_command_registry_}));

  CreateEventAndDataServices(ctx);
  CreateUserServices(ctx);
  CreateFeatureComponents(ctx);
  RunModuleConfigurator(ctx);
  CreateMainWindow(ctx);

  // TODO: Move selection command registry out of `MainWindowModule`.
  filesystem_component_->set_selection_commands(
      &core_module_->selection_commands());

  filesystem_component_->StartUp();
  co_return;
}

void ClientApplication::CreateNodeService(const PostLoginContext& ctx) {
  if (node_service_override_) {
    node_service_ = std::move(node_service_override_);
  } else {
    node_service_ = ::CreateNodeService(NodeServiceContext{
        .executor_ = executor_,
        .session_service_ = *ctx.audited_scada_services.session_service,
        .attribute_service_ = *ctx.audited_scada_services.attribute_service,
        .view_service_ = *ctx.audited_scada_services.view_service,
        .monitored_item_service_ =
            *ctx.audited_scada_services.monitored_item_service,
        .method_service_ = *ctx.audited_scada_services.method_service,
        .scada_client_ = ctx.scada_client});
  }
  shutdown_stack_.Push([this] { node_service_.reset(); });
}

void ClientApplication::CreateEventAndDataServices(
    const PostLoginContext& ctx) {
  // `profile_` is created and filled by LoadProfileAsync, which PostLoginAsync
  // runs before this: the profile has to be final before anything takes a
  // reference to it.
  event_module_ = std::make_unique<EventModule>(EventModuleContext{
      .executor_ = executor_,
      .logger_ = logger_,
      .profile_ = *profile_,
      .services_ = ctx.audited_scada_services,
      .controller_registry_ = *controller_registry_,
      .global_commands_ = core_module_->global_commands(),
      .selection_commands_ = core_module_->selection_commands(),
      .ui_command_registry_ = *ui_command_registry_});

  if (timed_data_service_override_) {
    timed_data_service_ = std::move(timed_data_service_override_);
  } else {
    timed_data_service_ = CreateTimedDataService(CoroutineTimedDataContext{
        .executor_ = executor_,
        .alias_resolver_ = ctx.alias_resolver,
        .node_service_ = *node_service_,
        .history_service_ = ctx.coroutine_history_service,
        .node_event_provider_ = event_module_->node_event_provider()});
  }
  shutdown_stack_.Push([this] { timed_data_service_.reset(); });
}

void ClientApplication::CreateUserServices(const PostLoginContext& ctx) {
  task_manager_ = std::make_shared<TaskManagerImpl>(TaskManagerImplContext{
      .executor_ = executor_,
      .node_service_ = *node_service_,
      .attribute_service_ = *ctx.audited_scada_services.attribute_service,
      .node_management_service_ =
          *ctx.audited_scada_services.node_management_service,
      .local_events_ = event_module_->local_events(),
      .profile_ = *profile_,
      .progress_host_ = core_module_->progress_host()});
  shutdown_stack_.Push([this] { task_manager_.reset(); });

  speech_ = std::make_unique<Speech>();
  shutdown_stack_.Push([this] { speech_.reset(); });

  blinker_manager_ = std::make_unique<BlinkerManagerImpl>(executor_);
  shutdown_stack_.Push([this] { blinker_manager_.reset(); });

  connection_state_reporter_ =
      std::make_unique<ConnectionStateReporter>(ConnectionStateReporterContext{
          .executor_ = executor_,
          .session_service_ = *ctx.audited_scada_services.session_service,
          .local_events_ = event_module_->local_events()});
  shutdown_stack_.Push([this] { connection_state_reporter_.reset(); });

  // Held for the whole logged-in lifetime rather than only while
  // `IsConnected()` — the reconnect backoff in `ConnectionStateReporter` runs
  // on the same stallable timer, so releasing the assertion on a dropped
  // session would leave the client unable to reconnect until someone raised its
  // window. No-op off macOS; see services/app_nap_suppressor.h.
  app_nap_suppressor_ = std::make_unique<AppNapSuppressor>();
  shutdown_stack_.Push([this] { app_nap_suppressor_.reset(); });

  write_service_ = std::make_unique<WriteServiceImpl>(
      WriteServiceImplContext{.executor_ = executor_,
                              .timed_data_service_ = *timed_data_service_,
                              .profile_ = *profile_});

  singletons_.emplace(std::make_shared<WriteModule>(WriteModuleContext{
      .executor_ = executor_,
      .timed_data_service_ = *timed_data_service_,
      .session_service_ = *ctx.audited_scada_services.session_service,
      .profile_ = *profile_,
      .selection_commands_ = core_module_->selection_commands(),
      .ui_command_registry_ = *ui_command_registry_}));

  create_tree_ = std::make_unique<CreateTree>();
  shutdown_stack_.Push([this] { create_tree_.reset(); });

  singletons_.emplace(std::make_shared<LimitsModule>(LimitsModuleContext{
      .executor_ = executor_,
      .session_service_ = *ctx.audited_scada_services.session_service,
      .task_manager_ = *task_manager_,
      .selection_commands_ = core_module_->selection_commands(),
      .ui_command_registry_ = *ui_command_registry_}));
}

void ClientApplication::CreateFeatureComponents(const PostLoginContext& ctx) {
  filesystem_component_ =
      std::make_unique<FileSystemComponent>(FileSystemComponentContext{
          .executor_ = executor_,
          .node_service_ = *node_service_,
          .task_manager_ = *task_manager_,
          .create_tree_ = *create_tree_,
          .default_node_commands_ = default_node_commands_,
          .global_commands_ = core_module_->global_commands(),
          .ui_command_registry_ = *ui_command_registry_,
          .scada_client_ = ctx.scada_client});

  favorites_module_ = std::make_unique<FavoritesModule>(FavoritesModuleContext{
      .executor_ = executor_,
      .profile_ = *profile_,
      .global_commands_ = core_module_->global_commands(),
      .controller_registry_ = *controller_registry_,
      .ui_command_registry_ = *ui_command_registry_});
  shutdown_stack_.Push([this] { favorites_module_.reset(); });

  // The rail's Administration mode pane. Registered unconditionally; the
  // WIN_REQUIRES_ADMIN flag on its WindowInfo is what hides the mode from a
  // session without the Configure right.
  administration_module_ =
      std::make_unique<AdministrationModule>(AdministrationModuleContext{
          .controller_registry_ = *controller_registry_});
  shutdown_stack_.Push([this] { administration_module_.reset(); });

  portfolio_module_ = std::make_unique<PortfolioModule>(PortfolioModuleContext{
      *node_service_, *profile_, *controller_registry_, *ui_command_registry_});
  shutdown_stack_.Push([this] { portfolio_module_.reset(); });

  property_service_ = std::make_unique<PropertyService>(executor_);
}

ClientApplicationModuleContext ClientApplication::BuildModuleContext(
    const PostLoginContext& ctx) {
  return ClientApplicationModuleContext{
      .executor_ = executor_,
      .scada_services_ = ctx.audited_scada_services,
      .alias_resolver_ = ctx.alias_resolver,
      .controller_registry_ = *controller_registry_,
      .profile_ = *profile_,
      .node_service_ = *node_service_,
      .task_manager_ = *task_manager_,
      .timed_data_service_ = *timed_data_service_,
      .local_events_ = event_module_->local_events(),
      .write_service_ = *write_service_,
      .print_module_ = print_module_,
      .node_service_tree_factory_ =
          node_service_tree_factory_
              ? node_service_tree_factory_
              : NodeServiceTreeFactory{[](NodeServiceTreeImplContext&& inner) {
                  return std::make_unique<NodeServiceTreeImpl>(
                      std::move(inner));
                }},
      .filesystem_component_ = *filesystem_component_,
      .blinker_manager_ = *blinker_manager_,
      .progress_host_ = core_module_->progress_host(),
      .global_commands_ = core_module_->global_commands(),
      .selection_commands_ = core_module_->selection_commands(),
      .ui_command_registry_ = *ui_command_registry_,
      .opened_view_commands_ = *opened_view_command_registry_,
      .singletons_ = singletons_};
}

void ClientApplication::RunModuleConfigurator(const PostLoginContext& ctx) {
  if (!module_configurator_) {
    return;
  }
  auto module_context = BuildModuleContext(ctx);
  module_configurator_(module_context);
  shutdown_stack_.Push([this] { print_module_.reset(); });
  shutdown_stack_.Push([this] { node_service_progress_tracker_.reset(); });
}

void ClientApplication::CreateMainWindow(const PostLoginContext& ctx) {
  auto controller_factory =
      std::make_shared<ControllerFactoryImpl>(ControllerFactoryImpl{
          .executor_ = executor_,
          .profile_ = *profile_,
          .scada_services_ = ctx.audited_scada_services,
          .task_manager_ = *task_manager_,
          .node_event_provider_ = event_module_->node_event_provider(),
          .timed_data_service_ = *timed_data_service_,
          .node_service_ = *node_service_,
          .file_cache_ = filesystem_component_->file_cache(),
          .blinker_manager_ = *blinker_manager_,
          .property_service_ = *property_service_,
          .create_tree_ = *create_tree_,
          .frame_capture_registry_ = frame_capture_registry_});

  main_window_module_ =
      std::make_unique<MainWindowModule>(MainWindowModuleContext{
          .executor_ = executor_,
          .profile_ = *profile_,
          .quit_handler_ =
              [this] { CoSpawn(executor_, [this] { return Quit(); }); },
          .scada_services_ = ctx.audited_scada_services,
          .login_handler_ =
              [this] { CoSpawn(executor_, [this] { return Login(); }); },
          .task_manager_ = *task_manager_,
          .node_event_provider_ = event_module_->node_event_provider(),
          .timed_data_service_ = *timed_data_service_,
          .node_service_ = *node_service_,
          .print_service_ =
              print_module_ ? &print_module_->print_service() : nullptr,
          .portfolio_manager_ = portfolio_module_->portfolio_manager(),
          .local_events_ = event_module_->local_events(),
          .frame_capture_registry_ = frame_capture_registry_,
          .favourites_ = favorites_module_->favourites(),
          .file_cache_ = filesystem_component_->file_cache(),
          .file_manager_ = filesystem_component_->file_manager(),
          .speech_service_ = *speech_,
          .node_command_handler_ = std::bind_front(
              &DefaultNodeCommandRegistry::Execute, &default_node_commands_),
          .default_node_commands_ = default_node_commands_,
          .progress_host_ = core_module_->progress_host(),
          .create_tree_ = *create_tree_,
          .global_commands_ = core_module_->global_commands(),
          .selection_commands_ = core_module_->selection_commands(),
          .ui_command_registry_ = *ui_command_registry_,
          .opened_view_commands_ = *opened_view_command_registry_,
          .controller_factory_ = std::bind_front(
              &ControllerFactoryImpl::CreateController, controller_factory)});
  shutdown_stack_.Push([this] { main_window_module_.reset(); });
}

Awaitable<void> ClientApplication::Login() {
  return LoginAsync();
}

Awaitable<void> ClientApplication::LoginAsync() {
  LOG_INFO(*logger_) << ("Login");

  DataServicesContext services_context{logger_, executor_, *transport_factory_,
                                       ReadServiceLogParamsFromCommandLine()};

  auto services = co_await login_handler_(std::move(services_context));
  if (!services) {
    throw LoginCanceled{};
  }
  OnLoginCompleted(std::move(*services));
  co_return;
}

void ClientApplication::OnLoginCompleted(const DataServices& data_services) {
  LOG_INFO(*logger_) << ("Login completed");

  auto audited_services =
      *AuditDataServices(data_services, core_module_->tracer(), executor_);
  master_data_services_->SetServices(std::move(audited_services));
}

Awaitable<void> ClientApplication::Quit() {
  if (quitting_) {
    co_await RunAsync();
    co_return;
  }

  quitting_ = true;
  CoSpawn(executor_, [this]() -> Awaitable<void> {
    try {
      co_await QuitAsync();
      quit_completion_.Complete();
    } catch (...) {
      quit_completion_.Fail(std::current_exception());
    }
  });
  co_await RunAsync();
}

Awaitable<void> ClientApplication::Run() {
  return RunAsync();
}

Awaitable<void> ClientApplication::RunAsync() {
  co_await quit_completion_.Wait();
}

Awaitable<void> ClientApplication::QuitAsync() {
  LOG_INFO(*logger_) << ("Quit");

  if (!master_data_services_) {
    co_return;
  }

  // **Here, and not in ~ClientApplication.** The destructor is where the local
  // file is written, and it cannot host this one: a destructor cannot
  // `co_await`. This runs while the session and the services are still alive,
  // which is the whole requirement -- one step later, after Disconnect, there
  // is nothing to write to.
  co_await SaveProfileToServerOnQuitAsync();

  LOG_INFO(*logger_) << ("Disconnect");

  // Event module is not created if login fails.
  // TODO: Create event module unconditionally.
  if (event_module_) {
    // TODO: Localize.
    event_module_->local_events().ReportEvent(
        LocalEvents::SEV_ERROR, Translate("Disconnecting from server..."));
  }

  try {
    co_await master_data_services_->Disconnect();
  } catch (...) {
    // Matches the prior IgnoreResult(): disconnect errors must not prevent
    // the application from quitting.
  }
  co_return;
}
