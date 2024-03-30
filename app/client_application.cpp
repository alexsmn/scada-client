#include "app/client_application.h"

#include "base/blinker.h"
#include "base/boost_log_adapter.h"
#include "base/command_line.h"
#include "base/promise_executor.h"
#include "common/audit.h"
#include "common/master_data_services.h"
#include "common_resources.h"
#include "components/debugger/debugger_module.h"
#include "components/write/write_service_impl.h"
#include "configuration/configuration_module.h"
#include "controller/controller_factory_impl.h"
#include "controller/controller_registry.h"
#include "core/core_module.h"
#include "events/event_module.h"
#include "events/local_events.h"
#include "export/configuration/export_configuration_module.h"
#include "export/csv/csv_export_module.h"
#include "favorites/favorites_module.h"
#include "filesystem/filesystem_component.h"
#include "main_window/main_window_module.h"
#include "main_window/main_window_util.h"
#include "metrics/boost_log_metric_reporter.h"
#include "metrics/metric_service_impl.h"
#include "node_service/node_service_factory.h"
#include "node_service_progress_tracker.h"
#include "portfolio/portfolio_module.h"
#include "profile/profile.h"
#include "project.h"
#include "properties/property_service.h"
#include "remote/remote_services.h"
#include "scada/service_context.h"
#include "services/alias_resolver_factory.h"
#include "services/connection_state_reporter.h"
#include "services/create_tree.h"
#include "services/progress_host_impl.h"
#include "services/speech.h"
#include "services/task_manager_impl.h"
#include "timed_data/timed_data_service_factory.h"

#if !defined(UI_WT)
#include "modus/modus_module.h"
#include "vidicon/vidicon_module.h"
#endif

#include <net/transport_factory_impl.h>

using namespace std::chrono_literals;

namespace {

scada::ServiceLogParams ReadServiceLogParamsFromCommandLine() {
  assert(base::CommandLine::ForCurrentProcess());
  const auto& command_line = *base::CommandLine::ForCurrentProcess();

  return {.log_read = command_line.HasSwitch("log-service-read"),
          .log_browse = command_line.HasSwitch("log-service-browse"),
          .log_history = command_line.HasSwitch("log-service-history"),
          .log_event = command_line.HasSwitch("log-service-event"),
          .log_model_change_event =
              command_line.HasSwitch("log-service-model-change-event"),
          .log_node_semantics_change_event = command_line.HasSwitch(
              "log-service-node-semantics-change-event")};
}

}  // namespace

extern bool CreateVidiconServices(const DataServicesContext& context,
                                  DataServices& services);
extern bool CreateOpcUaServices(const DataServicesContext& context,
                                DataServices& services);

REGISTER_DATA_SERVICES("Scada",
                       u"Телеконтроль",
                       CreateRemoteServices,
                       "localhost");

REGISTER_DATA_SERVICES("OpcUa",
                       u"OPC UA",
                       CreateOpcUaServices,
                       "opc.tcp://localhost:4840");

REGISTER_DATA_SERVICES("Vidicon",
                       u"Видикон",
                       CreateVidiconServices,
                       "localhost");

ClientApplication::ClientApplication(ClientApplicationContext&& context)
    : ClientApplicationContext{std::move(context)},
      metric_service_{
          std::make_unique<MetricServiceImpl>(executor_,
                                              /*report_metric_period*/ 1min)},
      controller_registry_{std::make_unique<ControllerRegistry>()},
      master_data_services_{std::make_shared<MasterDataServices>()} {
  logger_ = std::make_shared<BoostLogAdapter>("client");

  metric_service_->RegisterSink(
      [reporter = BoostLogMetricReporter{}](const Metrics& metrics) mutable {
        reporter.Report(metrics);
      });

  transport_factory_ = std::make_unique<net::TransportFactoryImpl>(io_context_);

  core_module_ = std::make_unique<CoreModule>(executor_);
}

ClientApplication::~ClientApplication() {
  node_service_progress_tracker_.reset();
  main_window_module_.reset();

  if (profile_ && profile_loaded_) {
    profile_->Save();
  }

  // Shutdown OPC.
  // extern void ShutdownOpc();
  // ShutdownOpc();

  while (!singletons_.empty()) {
    singletons_.pop();
  }

  connection_state_reporter_.reset();
  blinker_manager_.reset();
  speech_.reset();
  favorites_module_.reset();
  portfolio_module_.reset();
  task_manager_.reset();

  profile_.reset();

  create_tree_.reset();
  timed_data_service_.reset();
  node_service_.reset();

  master_data_services_ = nullptr;

  core_module_.reset();
  transport_factory_.reset();
}

MainWindowManager& ClientApplication::main_window_manager() {
  return main_window_module_->main_window_manager();
}

promise<void> ClientApplication::Start() {
  return Login().then(
      BindPromiseExecutor(executor_, [this] { StartAfterLoginCompleted(); }));
}

void ClientApplication::StartAfterLoginCompleted() {
  scada::client scada_client{master_data_services_->as_services()};

  node_service_ = CreateNodeService(
      NodeServiceContext{.executor_ = executor_,
                         .session_service_ = *master_data_services_,
                         .attribute_service_ = *master_data_services_,
                         .view_service_ = *master_data_services_,
                         .monitored_item_service_ = *master_data_services_,
                         .method_service_ = *master_data_services_,
                         .scada_client_ = scada_client});

  AliasResolver alias_resolver = CreateAliasResolver(*node_service_, logger_);

  singletons_.emplace(
      std::make_shared<CsvExportModule>(CsvExportModuleContext{}));

  profile_ = std::make_unique<Profile>();
  profile_->Load();
  profile_loaded_ = true;

  event_module_ = std::make_unique<EventModule>(EventModuleContext{
      .executor_ = executor_,
      .logger_ = logger_,
      .profile_ = *profile_,
      .services_ = master_data_services_->as_services(),
      .controller_registry_ = *controller_registry_,
      .selection_commands_ = core_module_->selection_commands()});

  timed_data_service_ = CreateTimedDataService(
      {.executor_ = executor_,
       .alias_resolver_ = alias_resolver,
       .node_service_ = *node_service_,
       .services_ = master_data_services_->as_services(),
       .node_event_provider_ = event_module_->node_event_provider()});

  auto progress_host = std::make_shared<ProgressHostImpl>();
  singletons_.emplace(progress_host);

  task_manager_ = std::make_shared<TaskManagerImpl>(
      TaskManagerImplContext{.executor_ = executor_,
                             .node_service_ = *node_service_,
                             .attribute_service_ = *master_data_services_,
                             .node_management_service_ = *master_data_services_,
                             .local_events_ = event_module_->local_events(),
                             .profile_ = *profile_,
                             .progress_host_ = *progress_host});

  speech_ = std::make_unique<Speech>();
  blinker_manager_ = std::make_unique<BlinkerManagerImpl>(executor_);

  connection_state_reporter_ =
      std::make_unique<ConnectionStateReporter>(ConnectionStateReporterContext{
          .executor_ = executor_,
          .session_service_ = *master_data_services_,
          .local_events_ = event_module_->local_events()});

  write_service_ = std::make_unique<WriteServiceImpl>(
      WriteServiceImplContext{.executor_ = executor_,
                              .timed_data_service_ = *timed_data_service_,
                              .profile_ = *profile_});

  singletons_.emplace(std::make_shared<ConfigurationModule>(
      ConfigurationModuleContext{.controller_registry_ = *controller_registry_,
                                 .profile_ = *profile_}));

  singletons_.emplace(std::make_shared<DebuggerModule>(DebuggerModuleContext{
      .session_service_ = *master_data_services_,
      .global_commands_ = core_module_->global_commands(),
      .selection_commands_ = core_module_->selection_commands()}));

  filesystem_component_ = std::make_unique<FileSystemComponent>(
      FileSystemComponentContext{.node_service_ = *node_service_,
                                 .task_manager_ = *task_manager_,
                                 .create_tree_ = *create_tree_,
                                 .scada_client_ = scada_client});

  favorites_module_ = std::make_unique<FavoritesModule>(FavoritesModuleContext{
      .profile_ = *profile_,
      .global_commands_ = core_module_->global_commands(),
      .controller_registry_ = *controller_registry_});

  portfolio_module_ = std::make_unique<PortfolioModule>(
      PortfolioModuleContext{*node_service_, *profile_, *controller_registry_});

  create_tree_ = std::make_unique<CreateTree>();

  property_service_ = std::make_unique<PropertyService>();

#if !defined(UI_WT)
  singletons_.emplace(std::make_shared<ModusModule>(ModusModuleContext{
      .controller_registry_ = *controller_registry_,
      .blinker_manager_ = *blinker_manager_,
      .file_registry_ = filesystem_component_->file_registry(),
      .global_commands_ = core_module_->global_commands(),
      .profile_ = *profile_,
      .alias_resolver_ = alias_resolver}));

  singletons_.emplace(std::make_shared<VidiconModule>(VidiconModuleContext{
      .executor_ = executor_,
      .timed_data_service_ = *timed_data_service_,
      .controller_registry_ = *controller_registry_,
      .write_service_ = *write_service_,
      .file_registry_ = filesystem_component_->file_registry()}));
#endif

  singletons_.emplace(std::make_shared<ExportConfigurationModule>(
      ExportConfigurationModuleContext{
          .node_service_ = *node_service_,
          .task_manager_ = *task_manager_,
          .global_commands_ = core_module_->global_commands()}));

  singletons_.emplace(std::make_shared<NodeServiceProgressTracker>(
      executor_, *node_service_, *progress_host));

  auto controller_factory =
      std::make_shared<ControllerFactoryImpl>(ControllerFactoryImpl{
          .executor_ = executor_,
          .profile_ = *profile_,
          .master_data_services_ = *master_data_services_,
          .task_manager_ = *task_manager_,
          .node_event_provider_ = event_module_->node_event_provider(),
          .timed_data_service_ = *timed_data_service_,
          .node_service_ = *node_service_,
          .file_cache_ = filesystem_component_->file_cache(),
          .blinker_manager_ = *blinker_manager_,
          .property_service_ = *property_service_,
          .create_tree_ = *create_tree_});

  main_window_module_ =
      std::make_unique<MainWindowModule>(MainWindowModuleContext{
          .executor_ = executor_,
          .profile_ = *profile_,
          .quit_handler_ = std::bind_front(&ClientApplication::Quit, this),
          .master_data_services_ = *master_data_services_,
          .login_handler_ = std::bind_front(&ClientApplication::Login, this),
          .task_manager_ = *task_manager_,
          .event_fetcher_ = event_module_->event_fetcher(),
          .timed_data_service_ = *timed_data_service_,
          .node_service_ = *node_service_,
          .portfolio_manager_ = portfolio_module_->portfolio_manager(),
          .local_events_ = event_module_->local_events(),
          .favourites_ = favorites_module_->favourites(),
          .file_cache_ = filesystem_component_->file_cache(),
          .file_manager_ = filesystem_component_->file_manager(),
          .speech_ = *speech_,
          .node_command_handler_ =
              std::bind_front(&::ExecuteDefaultNodeCommand, executor_,
                              filesystem_component_->file_command()),
          .progress_host_ = *progress_host,
          .create_tree_ = *create_tree_,
          .global_commands_ = core_module_->global_commands(),
          .selection_commands_ = core_module_->selection_commands(),
          .controller_factory_ = std::bind_front(
              &ControllerFactoryImpl::CreateController, controller_factory)});

  // TODO: Move selection command registry out of `MainWindowModule`.
  filesystem_component_->set_selection_commands(
      &core_module_->selection_commands());

  filesystem_component_->StartUp();
}

promise<void> ClientApplication::Login() {
  logger_->Write(LogSeverity::Normal, "Login");

  DataServicesContext services_context{logger_, executor_, *transport_factory_,
                                       ReadServiceLogParamsFromCommandLine()};

  return login_handler_(std::move(services_context))
      .then(BindPromiseExecutor(executor_,
                                [this](std::optional<DataServices> services) {
                                  if (!services) {
                                    throw std::runtime_error{"Login failed"};
                                  }
                                  OnLoginCompleted(std::move(*services));
                                }));
}

void ClientApplication::OnLoginCompleted(DataServices services) {
  logger_->Write(LogSeverity::Normal, "Login completed");

  // |Audit| doesn't own underlying services.
  struct Holder {
    Holder(MetricService& metric_service,
           DataServices data_services,
           Tracer& tracer)
        : data_services_{std::move(data_services)},
          audit_{Audit::Create(
              AuditContext{metric_service, *data_services_.attribute_service_,
                           *data_services_.view_service_, tracer})} {}

    DataServices data_services_;
    std::shared_ptr<Audit> audit_;
  };

  assert(core_module_);

  auto holder = std::make_shared<Holder>(*metric_service_, services,
                                         core_module_->tracer());

  std::shared_ptr<Audit> audit{holder, holder->audit_.get()};

  services.attribute_service_ = audit;
  services.view_service_ = audit;

  master_data_services_->SetServices(std::move(services));
}

promise<void> ClientApplication::Quit() {
  logger_->Write(LogSeverity::Normal, "Quit");

  if (!master_data_services_) {
    quit_promise_.resolve();
    return quit_promise_;
  }

  // TODO: Localize.
  event_module_->local_events().ReportEvent(LocalEvents::SEV_ERROR,
                                            u"Отключение от сервера...");

  logger_->Write(LogSeverity::Normal, "Disconnect");

  ForwardPromise(IgnoreResult(master_data_services_->Disconnect()),
                 quit_promise_);
  return quit_promise_;
}
