#include "app/client_application.h"

#include "aui/translation.h"
#include "base/awaitable_promise.h"
#include "base/blinker.h"
#include "base/executor_conversions.h"
#include "base/boost_log_adapter.h"
#include "base/program_options.h"
#include "base/promise_executor.h"
#include "common/audit.h"
#include "common/master_data_services.h"
#include "resources/common_resources.h"
#include "components/write/write_service_impl.h"
#include "configuration/tree/node_service_tree_impl.h"
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
#include "services/ui/node_service_progress_tracker.h"
#include "portfolio/portfolio_module.h"
#include "print/service/print_module.h"
#include "profile/profile.h"
#include "project.h"
#include "properties/property_service.h"
#include "remote/remote_services.h"
#include "scada/service_context.h"
#include "services/alias_resolver_factory.h"
#include "services/connection_state_reporter.h"
#include "services/create_tree.h"
#include "services/speech_service_impl.h"
#include "services/task_manager_impl.h"
#include "timed_data/timed_data_service_factory.h"

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

}  // namespace

extern bool CreateVidiconServices(const DataServicesContext& context,
                                  DataServices& services);

REGISTER_DATA_SERVICES("Scada",
                       u"Telecontrol",
                       CreateRemoteServices,
                       "localhost");

REGISTER_DATA_SERVICES("Vidicon",
                       u"Vidicon",
                       CreateVidiconServices,
                       "localhost");

// Locals shared across the PostLogin() phase helpers.
struct ClientApplication::PostLoginContext {
  scada::services audited_scada_services;
  scada::client scada_client;
  AliasResolver alias_resolver;
};

ClientApplication::ClientApplication(ClientApplicationContext&& context)
    : ClientApplicationContext{std::move(context)},
      metric_service_{
          std::make_unique<MetricServiceImpl>(MakeAnyExecutor(executor_),
                                              /*report_metric_period*/ 1min)},
      controller_registry_{std::make_unique<ControllerRegistry>()},
      master_data_services_{std::make_shared<MasterDataServices>()} {
  logger_ = std::make_shared<BoostLogAdapter>("client");

  metric_service_->RegisterSink(
      [reporter = BoostLogMetricReporter{}](const Metrics& metrics) mutable {
        reporter.Report(metrics);
      });

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

bool ClientApplication::HasSelectionCommandForTesting(
    unsigned command_id) const {
  return core_module_ &&
         core_module_->selection_commands().FindCommand(command_id) != nullptr;
}

promise<void> ClientApplication::Start() {
  return ToPromise(NetExecutorAdapter{executor_}, StartAsync());
}

Awaitable<void> ClientApplication::StartAsync() {
  co_await LoginAsync();
  PostLogin();
  co_return;
}

void ClientApplication::PostLogin() {
  PostLoginContext ctx{
      .audited_scada_services = master_data_services_->as_services(),
      .scada_client = scada::client{master_data_services_->as_services()},
      .alias_resolver = {}};

  CreateNodeService(ctx);
  ctx.alias_resolver = CreateAliasResolver(*node_service_, logger_);

  singletons_.emplace(
      std::make_shared<CsvExportModule>(CsvExportModuleContext{}));

  CreateEventAndDataServices(ctx);
  CreateUserServices(ctx);
  CreateFeatureComponents(ctx);
  RunModuleConfigurator(ctx);
  CreateMainWindow(ctx);

  // TODO: Move selection command registry out of `MainWindowModule`.
  filesystem_component_->set_selection_commands(
      &core_module_->selection_commands());

  filesystem_component_->StartUp();
}

void ClientApplication::CreateNodeService(const PostLoginContext& ctx) {
  if (node_service_override_) {
    node_service_ = std::move(node_service_override_);
  } else {
    node_service_ = ::CreateNodeService(NodeServiceContext{
        .executor_ = MakeAnyExecutor(executor_),
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

void ClientApplication::CreateEventAndDataServices(const PostLoginContext& ctx) {
  profile_ = std::make_unique<Profile>();
  profile_->Load();
  profile_loaded_ = true;
  shutdown_stack_.Push([this] { profile_.reset(); });

  event_module_ = std::make_unique<EventModule>(EventModuleContext{
      .executor_ = executor_,
      .logger_ = logger_,
      .profile_ = *profile_,
      .services_ = ctx.audited_scada_services,
      .controller_registry_ = *controller_registry_,
      .selection_commands_ = core_module_->selection_commands()});

  if (timed_data_service_override_) {
    timed_data_service_ = std::move(timed_data_service_override_);
  } else {
    timed_data_service_ = CreateTimedDataService(
        {.executor_ = MakeAnyExecutor(executor_),
         .alias_resolver_ = ctx.alias_resolver,
         .node_service_ = *node_service_,
         .services_ = ctx.audited_scada_services,
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

  write_service_ = std::make_unique<WriteServiceImpl>(
      WriteServiceImplContext{.executor_ = executor_,
                              .timed_data_service_ = *timed_data_service_,
                              .profile_ = *profile_});

  create_tree_ = std::make_unique<CreateTree>();
  shutdown_stack_.Push([this] { create_tree_.reset(); });
}

void ClientApplication::CreateFeatureComponents(const PostLoginContext& ctx) {
  filesystem_component_ = std::make_unique<FileSystemComponent>(
      FileSystemComponentContext{.executor_ = executor_,
                                 .node_service_ = *node_service_,
                                 .task_manager_ = *task_manager_,
                                 .create_tree_ = *create_tree_,
                                 .scada_client_ = ctx.scada_client});

  favorites_module_ = std::make_unique<FavoritesModule>(FavoritesModuleContext{
      .profile_ = *profile_,
      .global_commands_ = core_module_->global_commands(),
      .controller_registry_ = *controller_registry_});
  shutdown_stack_.Push([this] { favorites_module_.reset(); });

  portfolio_module_ = std::make_unique<PortfolioModule>(
      PortfolioModuleContext{*node_service_, *profile_, *controller_registry_});
  shutdown_stack_.Push([this] { portfolio_module_.reset(); });

  property_service_ = std::make_unique<PropertyService>();
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
      .write_service_ = *write_service_,
      .print_module_ = print_module_,
      .node_service_tree_factory_ =
          node_service_tree_factory_
              ? node_service_tree_factory_
              : NodeServiceTreeFactory{[](NodeServiceTreeImplContext&& inner) {
                  return std::make_unique<NodeServiceTreeImpl>(std::move(inner));
                }},
      .filesystem_component_ = *filesystem_component_,
      .blinker_manager_ = *blinker_manager_,
      .progress_host_ = core_module_->progress_host(),
      .global_commands_ = core_module_->global_commands(),
      .selection_commands_ = core_module_->selection_commands(),
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
          .create_tree_ = *create_tree_});

  main_window_module_ =
      std::make_unique<MainWindowModule>(MainWindowModuleContext{
          .executor_ = executor_,
          .profile_ = *profile_,
          .quit_handler_ = std::bind_front(&ClientApplication::Quit, this),
          .scada_services_ = ctx.audited_scada_services,
          .login_handler_ = std::bind_front(&ClientApplication::Login, this),
          .task_manager_ = *task_manager_,
          .node_event_provider_ = event_module_->node_event_provider(),
          .timed_data_service_ = *timed_data_service_,
          .node_service_ = *node_service_,
          .print_service_ =
              print_module_ ? &print_module_->print_service() : nullptr,
          .portfolio_manager_ = portfolio_module_->portfolio_manager(),
          .local_events_ = event_module_->local_events(),
          .favourites_ = favorites_module_->favourites(),
          .file_cache_ = filesystem_component_->file_cache(),
          .file_manager_ = filesystem_component_->file_manager(),
          .speech_service_ = *speech_,
          .node_command_handler_ =
              std::bind_front(&::ExecuteDefaultNodeCommand, executor_,
                              filesystem_component_->file_command()),
          .progress_host_ = core_module_->progress_host(),
          .create_tree_ = *create_tree_,
          .global_commands_ = core_module_->global_commands(),
          .selection_commands_ = core_module_->selection_commands(),
          .controller_factory_ = std::bind_front(
              &ControllerFactoryImpl::CreateController, controller_factory)});
  shutdown_stack_.Push([this] { main_window_module_.reset(); });
}

promise<void> ClientApplication::Login() {
  return ToPromise(NetExecutorAdapter{executor_}, LoginAsync());
}

Awaitable<void> ClientApplication::LoginAsync() {
  logger_->Write(LogSeverity::Normal, "Login");

  DataServicesContext services_context{logger_, executor_, *transport_factory_,
                                       ReadServiceLogParamsFromCommandLine()};

  auto services = co_await AwaitPromise(
      NetExecutorAdapter{executor_},
      login_handler_(std::move(services_context)));
  if (!services) {
    throw LoginCanceled{};
  }
  OnLoginCompleted(std::move(*services));
  co_return;
}

void ClientApplication::OnLoginCompleted(const DataServices& data_services) {
  logger_->Write(LogSeverity::Normal, "Login completed");

  auto audited_services =
      AuditScadaServices(data_services.CreateSharedServices(), *metric_service_,
                         core_module_->tracer());

  master_data_services_->SetServices(
      DataServices::FromSharedServices(audited_services));
}

promise<void> ClientApplication::Quit() {
  if (quitting_) {
    return quit_promise_;
  }

  quitting_ = true;
  StartAwaitable(NetExecutorAdapter{executor_}, quit_promise_, QuitAsync());
  return quit_promise_;
}

Awaitable<void> ClientApplication::QuitAsync() {
  logger_->Write(LogSeverity::Normal, "Quit");

  if (!master_data_services_) {
    co_return;
  }

  logger_->Write(LogSeverity::Normal, "Disconnect");

  // Event module is not created if login fails.
  // TODO: Create event module unconditionally.
  if (event_module_) {
    // TODO: Localize.
    event_module_->local_events().ReportEvent(
        LocalEvents::SEV_ERROR, Translate("Disconnecting from server..."));
  }

  try {
    co_await AwaitPromise(NetExecutorAdapter{executor_},
                          master_data_services_->Disconnect());
  } catch (...) {
    // Matches the prior IgnoreResult(): disconnect errors must not prevent
    // the application from quitting.
  }
  co_return;
}
