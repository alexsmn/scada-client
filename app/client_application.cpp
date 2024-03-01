#include "app/client_application.h"

#include "base/blinker.h"
#include "base/boost_log.h"
#include "base/boost_log_adapter.h"
#include "base/boost_log_init.h"
#include "base/client_paths.h"
#include "base/command_line.h"
#include "base/file_path_util.h"
#include "base/files/file_util.h"
#include "base/nested_logger.h"
#include "base/path_service.h"
#include "base/promise_executor.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/dump.h"
#include "common/audit.h"
#include "common/common_paths.h"
#include "common/master_data_services.h"
#include "common_resources.h"
#include "components/debugger/debugger_module.h"
#include "components/write/write_service_impl.h"
#include "configuration_tree/configuration_tree_module.h"
#include "controller/controller_factory_impl.h"
#include "controller/controller_registry.h"
#include "core/core_module.h"
#include "events/event_fetcher.h"
#include "events/event_fetcher_builder.h"
#include "export/configuration/export_configuration_module.h"
#include "export/csv/csv_export_module.h"
#include "favorites/favorites_module.h"
#include "filesystem/filesystem_component.h"
#include "main_window/main_window_manager.h"
#include "main_window/main_window_module.h"
#include "main_window/main_window_util.h"
#include "metrics/boost_log_metric_reporter.h"
#include "metrics/metric_service_impl.h"
#include "node_service/node_service.h"
#include "node_service/node_service_factory.h"
#include "node_service_progress_tracker.h"
#include "portfolio/portfolio_module.h"
#include "profile/profile.h"
#include "project.h"
#include "properties/property_service.h"
#include "remote/remote_services.h"
#include "services/alias_service.h"
#include "services/connection_state_reporter.h"
#include "services/create_tree.h"
#include "services/local_events.h"
#include "services/progress_host_impl.h"
#include "services/speech.h"
#include "services/task_manager_impl.h"
#include "timed_data/timed_data_service_impl.h"

#if !defined(UI_WT)
#include "modus/modus_module.h"
#include "vidicon/vidicon_module.h"
#endif

#include <net/transport_factory_impl.h>

using namespace std::chrono_literals;

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

namespace {

LONG WINAPI ProcessUnhandledException(_EXCEPTION_POINTERS* exception) {
  auto name = GetDumpFileName("client");

  base::FilePath base_path;
  base::PathService::Get(client::DIR_LOG, &base_path);
  auto path = AsFilesystemPath(base_path) / name;

  DumpException(path.c_str(), *exception);

  return EXCEPTION_EXECUTE_HANDLER;
}

}  // namespace

ClientApplication::ClientApplication(ClientApplicationContext&& context)
    : ClientApplicationContext{std::move(context)},
      metric_service_{
          std::make_unique<MetricServiceImpl>(executor_,
                                              /*report_metric_period*/ 1min)},
      controller_registry_{std::make_unique<ControllerRegistry>()},
      master_data_services_{std::make_shared<MasterDataServices>()} {
  if (!base::CommandLine::Init(0, nullptr))
    throw std::runtime_error{"Can't parse command line."};

  scada::RegisterPathProvider();
  client::RegisterPathProvider();

  // Initialize logging.
  {
    base::FilePath log_path;
    base::PathService::Get(client::DIR_LOG, &log_path);
    base::CreateDirectory(log_path);

    {
      auto path = log_path.Append(FILE_PATH_LITERAL("client.log"));
      logging::InitLogging(logging::LoggingSettings{
          .logging_dest = logging::LOG_TO_FILE,
          .log_file_path = path.value().c_str(),
          .lock_log = logging::LOCK_LOG_FILE,
          .delete_old = logging::APPEND_TO_OLD_LOG_FILE,
      });
    }

    {
      BoostLogParams params;
      params.path =
          log_path.Append(FILE_PATH_LITERAL("components.log")).value();
      InitBoostLogging(params);
    }
  }

  SetUnhandledExceptionFilter(ProcessUnhandledException);

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
    profile_->Save(*event_fetcher_);
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
  local_events_.reset();

  profile_.reset();

  create_tree_.reset();
  timed_data_service_.reset();
  event_fetcher_.reset();
  node_service_.reset();

  master_data_services_ = nullptr;

  core_module_.reset();
  transport_factory_.reset();

  ShutdownBoostLogging();
}

void ClientApplication::Start() {
  Login().then(BindExecutor(executor_, [this](bool ok) {
    if (ok)
      OnStartLoginCompleted();
    else
      quit_handler_();
  }));
}

void ClientApplication::OnStartLoginCompleted() {
  scada::client scada_client{
      scada::GetServices(master_data_services_->data_services())};

  event_fetcher_ =
      EventFetcherBuilder{.executor_ = executor_,
                          .logger_ = logger_,
                          .monitored_item_service_ = *master_data_services_,
                          .history_service_ = *master_data_services_,
                          .method_service_ = *master_data_services_,
                          .session_service_ = *master_data_services_}
          .Build();

  node_service_ = CreateNodeService(
      NodeServiceContext{.executor = executor_,
                         .session_service_ = *master_data_services_,
                         .attribute_service_ = *master_data_services_,
                         .view_service_ = *master_data_services_,
                         .monitored_item_service_ = *master_data_services_,
                         .method_service_ = *master_data_services_,
                         .scada_client_ = scada_client});

  auto alias_logger =
      base::CommandLine::ForCurrentProcess()->HasSwitch("log-alias-service")
          ? static_cast<std::shared_ptr<Logger>>(
                std::make_shared<NestedLogger>(logger_, "AliasService"))
          : static_cast<std::shared_ptr<Logger>>(
                std::make_shared<NullLogger>());

  auto alias_service = std::make_shared<AliasService>(
      AliasServiceContext{alias_logger, *node_service_});

  AliasResolver alias_resolver =
      std::bind_front(&AliasService::Resolve, alias_service);

  singletons_.emplace(
      std::make_shared<CsvExportModule>(CsvExportModuleContext{}));

  timed_data_service_ = std::make_unique<TimedDataServiceImpl>(TimedDataContext{
      .executor_ = executor_,
      .alias_resolver_ = alias_resolver,
      .node_service_ = *node_service_,
      .services_ = {.attribute_service = master_data_services_.get(),
                    .monitored_item_service = master_data_services_.get(),
                    .method_service = master_data_services_.get(),
                    .history_service = master_data_services_.get()},
      .node_event_provider_ = *event_fetcher_});

  profile_ = std::make_unique<Profile>();
  local_events_ = std::make_unique<LocalEvents>();

  auto progress_host = std::make_shared<ProgressHostImpl>();
  singletons_.emplace(progress_host);

  task_manager_ = std::make_shared<TaskManagerImpl>(
      TaskManagerImplContext{.executor_ = executor_,
                             .node_service_ = *node_service_,
                             .attribute_service_ = *master_data_services_,
                             .node_management_service_ = *master_data_services_,
                             .local_events_ = *local_events_,
                             .profile_ = *profile_,
                             .progress_host_ = *progress_host});

  speech_ = std::make_unique<Speech>();
  blinker_manager_ = std::make_unique<BlinkerManagerImpl>(executor_);

  connection_state_reporter_ = std::make_unique<ConnectionStateReporter>(
      ConnectionStateReporterContext{.executor_ = executor_,
                                     .session_service_ = *master_data_services_,
                                     .local_events_ = *local_events_});

  write_service_ = std::make_unique<WriteServiceImpl>(
      WriteServiceImplContext{.executor_ = executor_,
                              .timed_data_service_ = *timed_data_service_,
                              .profile_ = *profile_});

  singletons_.emplace(
      std::make_shared<ConfigurationTreeModule>(ConfigurationTreeModuleContext{
          .controller_registry_ = *controller_registry_,
          .profile_ = *profile_}));

  singletons_.emplace(std::make_shared<DebuggerModule>(
      DebuggerModuleContext{.session_service_ = *master_data_services_,
                            .main_commands_ = core_module_->main_commands()}));

  filesystem_component_ = std::make_unique<FileSystemComponent>(
      FileSystemComponentContext{.node_service_ = *node_service_,
                                 .task_manager_ = *task_manager_,
                                 .create_tree_ = *create_tree_,
                                 .scada_client_ = scada_client});

  profile_->Load(*event_fetcher_);
  profile_loaded_ = true;

  favorites_module_ = std::make_unique<FavoritesModule>(FavoritesModuleContext{
      .profile_ = *profile_, .main_commands_ = core_module_->main_commands()});

  portfolio_module_ = std::make_unique<PortfolioModule>(
      PortfolioModuleContext{*node_service_, *profile_});

  create_tree_ = std::make_unique<CreateTree>();

  property_service_ = std::make_unique<PropertyService>();

#if !defined(UI_WT)
  singletons_.emplace(std::make_shared<ModusModule>(ModusModuleContext{
      .controller_registry_ = *controller_registry_,
      .blinker_manager_ = *blinker_manager_,
      .file_registry_ = filesystem_component_->file_registry(),
      .main_commands_ = core_module_->main_commands(),
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
          .main_commands_ = core_module_->main_commands()}));

  singletons_.emplace(std::make_shared<NodeServiceProgressTracker>(
      executor_, *node_service_, *progress_host));

  auto controller_factory =
      std::make_shared<ControllerFactoryImpl>(ControllerFactoryImpl{
          .executor_ = executor_,
          .profile_ = *profile_,
          .master_data_services_ = *master_data_services_,
          .task_manager_ = *task_manager_,
          .node_event_provider_ = *event_fetcher_,
          .timed_data_service_ = *timed_data_service_,
          .node_service_ = *node_service_,
          .portfolio_manager_ = portfolio_module_->portfolio_manager(),
          .local_events_ = *local_events_,
          .favourites_ = favorites_module_->favourites(),
          .file_cache_ = filesystem_component_->file_cache(),
          .blinker_manager_ = *blinker_manager_,
          .property_service_ = *property_service_,
          .create_tree_ = *create_tree_});

  main_window_module_ =
      std::make_unique<MainWindowModule>(MainWindowModuleContext{
          .executor_ = executor_,
          .profile_ = *profile_,
          .main_window_factory_ = main_window_factory_,
          .quit_handler_ = std::bind_front(&ClientApplication::Quit, this),
          .master_data_services_ = *master_data_services_,
          .login_handler_ = std::bind_front(&ClientApplication::Login, this),
          .task_manager_ = *task_manager_,
          .event_fetcher_ = *event_fetcher_,
          .timed_data_service_ = *timed_data_service_,
          .node_service_ = *node_service_,
          .portfolio_manager_ = portfolio_module_->portfolio_manager(),
          .local_events_ = *local_events_,
          .favourites_ = favorites_module_->favourites(),
          .file_cache_ = filesystem_component_->file_cache(),
          .file_manager_ = filesystem_component_->file_manager(),
          .speech_ = *speech_,
          .node_command_handler_ =
              std::bind_front(&::ExecuteDefaultNodeCommand, executor_,
                              filesystem_component_->file_command()),
          .progress_host_ = *progress_host,
          .create_tree_ = *create_tree_,
          .main_commands_ = core_module_->main_commands(),
          .selection_commands_ = core_module_->selection_commands(),
          .controller_factory_ = std::bind_front(
              &ControllerFactoryImpl::CreateController, controller_factory)});

  // TODO: Move selection command registry out of `MainWindowModule`.
  filesystem_component_->set_selection_commands(
      &core_module_->selection_commands());

  filesystem_component_->StartUp();
}

promise<bool> ClientApplication::Login() {
  logger_->Write(LogSeverity::Normal, "Login");

  assert(base::CommandLine::ForCurrentProcess());
  const auto& command_line = *base::CommandLine::ForCurrentProcess();

  scada::ServiceLogParams service_log_params{
      command_line.HasSwitch("log-service-read"),
      command_line.HasSwitch("log-service-browse"),
      command_line.HasSwitch("log-service-history"),
      command_line.HasSwitch("log-service-event"),
      command_line.HasSwitch("log-service-model-change-event"),
      command_line.HasSwitch("log-service-node-semantics-change-event"),
  };

  DataServicesContext services_context{logger_, executor_, *transport_factory_,
                                       service_log_params};

  return login_handler_(std::move(services_context))
      .then(BindPromiseExecutor(executor_,
                                [this](std::optional<DataServices> services) {
                                  if (!services)
                                    return false;
                                  OnLoginCompleted(std::move(*services));
                                  return true;
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

void ClientApplication::Quit() {
  logger_->Write(LogSeverity::Normal, "Quit");

  if (!master_data_services_) {
    quit_handler_();
    return;
  }

  // TODO: Localize.
  local_events_->ReportEvent(LocalEvents::SEV_ERROR,
                             u"Отключение от сервера...");

  logger_->Write(LogSeverity::Normal, "Disconnect");

  master_data_services_->Disconnect().finally(quit_handler_);
}
