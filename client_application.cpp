#include "client_application.h"

#include "base/bind.h"
#include "base/blinker.h"
#include "base/boost_log.h"
#include "base/boost_log_adapter.h"
#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/nested_logger.h"
#include "base/path_service.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task_runner_executor.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/win/dump.h"
#include "client_paths.h"
#include "common/audit.h"
#include "common/audit_logger_impl.h"
#include "common/common_paths.h"
#include "common/event_fetcher.h"
#include "common/master_data_services.h"
#include "component_api_impl.h"
#include "components/events/events_component.h"
#include "components/favourites/favourites.h"
#include "components/filesystem/filesystem_component.h"
#include "components/login/login_dialog.h"
#include "components/main/action_manager.h"
#include "components/main/actions.h"
#include "components/main/context_menu_model.h"
#include "components/main/main_commands.h"
#include "components/main/main_menu_model.h"
#include "components/main/main_window.h"
#include "components/main/main_window_manager.h"
#include "components/main/opened_view_commands.h"
#include "components/main/selection_commands.h"
#include "components/main/status_bar_model_impl.h"
#include "components/modus/libmodus/modus_module2.h"
#include "components/portfolio/portfolio_manager.h"
#include "components/vidicon_display/vidicon_client.h"
#include "controller_context.h"
#include "net/transport_factory_impl.h"
#include "node_service/node_service.h"
#include "node_service/node_service_factory.h"
#include "project.h"
#include "services/alias_service.h"
#include "services/connection_state_reporter.h"
#include "services/event_dispatcher.h"
#include "services/file_cache.h"
#include "services/file_registry.h"
#include "services/local_events.h"
#include "services/profile.h"
#include "services/speech.h"
#include "services/task_manager_impl.h"
#include "timed_data/timed_data_service_impl.h"
#include "window_info.h"

extern bool CreateVidiconServices(const DataServicesContext& context,
                                  DataServices& services);
extern bool CreateScadaServices(const DataServicesContext& context,
                                DataServices& services);
extern bool CreateOpcUaServices(const DataServicesContext& context,
                                DataServices& services);

REGISTER_DATA_SERVICES("Scada", L"Телеконтроль", CreateScadaServices);
#ifndef NDEBUG
REGISTER_DATA_SERVICES("Vidicon", L"Видикон", CreateVidiconServices);
REGISTER_DATA_SERVICES("OpcUa", L"OPC UA", CreateOpcUaServices);
#endif

namespace {

LONG WINAPI ProcessUnhandledException(_EXCEPTION_POINTERS* exception) {
  SYSTEMTIME time;
  GetLocalTime(&time);

  base::FilePath path;
  base::PathService::Get(client::DIR_LOG, &path);

  // TODO: Take module name.
  std::wstring name = base::StringPrintf(
      L"client_%04d%02d%02d_%02d%02d%02d.dmp", time.wYear, time.wMonth,
      time.wDay, time.wHour, time.wMinute, time.wSecond);
  path = path.Append(name);

  DumpException(path.value().c_str(), *exception);

  return EXCEPTION_EXECUTE_HANDLER;
}

void RegisterFileType(FileRegistry& file_registry,
                      unsigned command_id,
                      std::string_view extensions) {
  auto* window_info = FindWindowInfo(command_id);
  assert(window_info);
  if (!window_info)
    return;

  file_registry.RegisterType(command_id, std::string{window_info->name},
                             extensions);
}

void PollIoContext(boost::asio::io_context* context) {
  context->poll();
}

}  // namespace

ClientApplication::ClientApplication(ClientApplicationContext&& context)
    : ClientApplicationContext{std::move(context)},
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
      logging::LoggingSettings log;
      log.logging_dest = logging::LOG_TO_FILE;
      log.log_file = path.value().c_str();
      log.lock_log = logging::LOCK_LOG_FILE;
      log.delete_old = logging::APPEND_TO_OLD_LOG_FILE;
      logging::InitLogging(log);
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

  io_context_ = std::make_unique<boost::asio::io_context>();
  executor_ =
      std::make_shared<TaskRunnerExecutor>(base::ThreadTaskRunnerHandle::Get());
  io_context_timer_ = std::make_unique<base::Timer>(true, true);
  io_context_timer_->Start(
      FROM_HERE, base::TimeDelta::FromMilliseconds(10),
      base::Bind(&PollIoContext, base::Unretained(io_context_.get())));

  transport_factory_ =
      std::make_unique<net::TransportFactoryImpl>(*io_context_);
}

ClientApplication::~ClientApplication() {
  event_dispatcher_.reset();
  main_window_manager_.reset();

  if (profile_ && profile_loaded_)
    profile_->Save(*event_fetcher_, *portfolio_manager_, *favourites_);

  VidiconClient::CleanupInstance();

  // Shutdown OPC.
  // extern void ShutdownOpc();
  // ShutdownOpc();

  ModusModule2::SetInstance(nullptr);
  modus_module_.reset();

  file_cache_.reset();
  connection_state_reporter_.reset();

  blinker_manager_.reset();
  speech_.reset();
  action_manager_.reset();
  favourites_.reset();
  portfolio_manager_.reset();
  task_manager_.reset();
  local_events_.reset();

  profile_.reset();

  master_data_services_->RemoveObserver(*this);

  timed_data_service_.reset();
  alias_resolver_ = nullptr;
  filesystem_component_.reset();
  event_fetcher_.reset();
  node_service_.reset();

  master_data_services_ = nullptr;

  transport_factory_.reset();
  io_context_timer_.reset();
  executor_.reset();
  io_context_.reset();
}

void ClientApplication::Start() {
  master_data_services_->AddObserver(*this);

  event_fetcher_ = std::make_unique<EventFetcher>(EventFetcherContext{
      executor_,
      *master_data_services_,
      *master_data_services_,
      *master_data_services_,
      std::make_shared<NestedLogger>(logger_, "EventFetcher"),
  });
  if (master_data_services_->IsConnected())
    event_fetcher_->OnChannelOpened(master_data_services_->GetUserId());

  node_service_ = CreateNodeService(NodeServiceContext{
      executor_,
      *master_data_services_,
      *master_data_services_,
      *master_data_services_,
      *master_data_services_,
      *master_data_services_,
  });

  auto alias_logger =
      base::CommandLine::ForCurrentProcess()->HasSwitch("log-alias-service")
          ? static_cast<std::shared_ptr<Logger>>(
                std::make_shared<NestedLogger>(logger_, "AliasService"))
          : static_cast<std::shared_ptr<Logger>>(
                std::make_shared<NullLogger>());
  auto alias_service = std::make_shared<AliasService>(AliasServiceContext{
      alias_logger,
      *node_service_,
  });
  alias_resolver_ = [alias_service](std::string_view alias,
                                    const AliasResolveCallback& callback) {
    alias_service->Resolve(alias, callback);
  };

  ComponentApiImpl component_api;
  filesystem_component_ = std::make_unique<FileSystemComponent>(component_api);

  timed_data_service_ = std::make_unique<TimedDataServiceImpl>(
      TimedDataContext{
          *io_context_,
          alias_resolver_,
          *node_service_,
          *master_data_services_,
          *master_data_services_,
          *master_data_services_,
          *master_data_services_,
          *master_data_services_,
          *event_fetcher_,
      },
      std::make_shared<NestedLogger>(logger_, "TimedDataService"));

  profile_ = std::make_unique<Profile>();
  local_events_ = std::make_unique<LocalEvents>();

  task_manager_ = std::make_shared<TaskManagerImpl>(TaskManagerImplContext{
      executor_,
      *node_service_,
      *master_data_services_,
      *master_data_services_,
      *local_events_,
      *profile_,
  });
  speech_.reset(new Speech);
  blinker_manager_ = std::make_unique<BlinkerManager>(executor_);

  connection_state_reporter_ = std::make_unique<ConnectionStateReporter>(
      ConnectionStateReporterContext{*master_data_services_, *local_events_});

  file_registry_ = std::make_unique<FileRegistry>();
  RegisterFileType(*file_registry_, ID_MODUS_VIEW, ".sde;.xsde");
  RegisterFileType(*file_registry_, ID_VIDICON_DISPLAY_VIEW, ".vds");

  file_cache_ = std::make_unique<FileCache>(*file_registry_);

  modus_module_ = std::make_unique<ModusModule2>(*blinker_manager_);
  ModusModule2::SetInstance(modus_module_.get());

  action_manager_ = std::make_unique<ActionManager>();
  AddGlobalActions(*action_manager_, *node_service_);

  favourites_ = std::make_unique<Favourites>();

  portfolio_manager_ = std::make_unique<PortfolioManager>(
      PortfolioManagerContext{*node_service_});

  profile_->Load(*event_fetcher_, *portfolio_manager_, *favourites_);
  profile_loaded_ = true;

  main_window_manager_ =
      std::make_unique<MainWindowManager>(MainWindowManagerContext{
          *profile_,
          [this](int window_id) {
            return main_window_factory_(MakeMainWindowContext(window_id));
          },
          [this] { Quit(); }});

  // |main_window_manager_| must be assigned.
  main_window_manager_->Init();

  event_dispatcher_ = std::make_unique<EventDispatcher>(EventDispatcherContext{
      executor_, *event_fetcher_, *local_events_, *profile_,
      [this](bool has_events) { OnEvents(has_events); }, *action_manager_});
}

MainWindowContext ClientApplication::MakeMainWindowContext(int window_id) {
  auto controller_factory =
      [this](unsigned command_id, ControllerDelegate& delegate,
             DialogService& dialog_service) -> std::unique_ptr<Controller> {
    auto* registrar = GetControllerRegistrar(command_id);
    if (!registrar)
      return nullptr;

    if (registrar->window_info().requires_admin_rights() &&
        !master_data_services_->HasPrivilege(scada::Privilege::Configure)) {
      return nullptr;
    }

    return registrar->CreateController(ControllerContext{
        executor_, delegate, alias_resolver_, *task_manager_,
        *master_data_services_, *event_fetcher_, *master_data_services_,
        *master_data_services_, *timed_data_service_, *node_service_,
        *portfolio_manager_, *local_events_, *favourites_, *file_cache_,
        *profile_, dialog_service, *blinker_manager_});
  };

  auto login_handler = [this](bool login) {
    if (login)
      Login();
    else
      master_data_services_->SetServices({});
  };

  auto main_commands_factory = [this, login_handler](
                                   MainWindow& main_window,
                                   DialogService& dialog_service) {
    return std::make_unique<MainCommands>(MainCommandsContext{
        main_window, *task_manager_, dialog_service, *master_data_services_,
        *event_fetcher_, *node_service_, *local_events_, *favourites_, *speech_,
        *profile_, *main_window_manager_, login_handler});
  };

  auto main_menu_factory =
      [this](MainWindow& main_window, DialogService& dialog_service,
             ViewManager& view_manager, CommandHandler& main_commands,
             ui::MenuModel& context_menu_model) {
        return std::make_unique<MainMenuModel>(MainMenuContext{
            *main_window_manager_, main_window, *action_manager_, *favourites_,
            *file_cache_,
            master_data_services_->HasPrivilege(scada::Privilege::Configure),
            *profile_, view_manager, main_commands, dialog_service,
            context_menu_model});
      };

  auto context_menu_factory = [this](MainWindow& main_window,
                                     CommandHandler& main_commands) {
    return std::make_unique<ContextMenuModel>(main_window, *action_manager_,
                                              main_commands);
  };

  auto selection_commands =
      std::make_shared<SelectionCommands>(SelectionCommandsContext{
          executor_, *task_manager_, *master_data_services_,
          *master_data_services_, *event_fetcher_, *timed_data_service_,
          *local_events_, *file_cache_, *profile_, *main_window_manager_,
          *node_service_});

  auto view_commands_factory = [this, selection_commands](
                                   OpenedView& opened_view,
                                   DialogService& dialog_service) {
    auto opened_view_commands =
        std::make_unique<OpenedViewCommands>(OpenedViewCommandsContext{
            executor_, selection_commands, *task_manager_,
            *master_data_services_, *master_data_services_, *event_fetcher_,
            *master_data_services_, *timed_data_service_, *node_service_,
            *portfolio_manager_, *action_manager_, *local_events_, *favourites_,
            *file_cache_, *profile_, *main_window_manager_});

    opened_view_commands->SetContext(&opened_view, &dialog_service);

    return opened_view_commands;
  };

  PendingTaskProvider pending_task_provider = [node_service = node_service_] {
    return static_cast<int>(node_service->GetPendingTaskCount());
  };

  auto status_bar_model =
      std::make_shared<StatusBarModelImpl>(StatusBarModelImplContext{
          *master_data_services_, *event_fetcher_, *node_service_,
          *task_manager_, std::move(pending_task_provider)});

  auto connection_info_provider = [this] {
    return master_data_services_->GetHostName();
  };

  return MainWindowContext{executor_,
                           *action_manager_,
                           window_id,
                           *file_registry_,
                           *file_cache_,
                           *main_window_manager_,
                           *profile_,
                           controller_factory,
                           main_commands_factory,
                           view_commands_factory,
                           selection_commands,
                           std::move(status_bar_model),
                           context_menu_factory,
                           main_menu_factory,
                           connection_info_provider};
}

void ClientApplication::OnSessionCreated() {
  if (event_fetcher_)
    event_fetcher_->OnChannelOpened(master_data_services_->GetUserId());
}

void ClientApplication::OnSessionDeleted(const scada::Status& status) {
  if (event_fetcher_)
    event_fetcher_->OnChannelClosed();
}

void ClientApplication::OnEvents(bool has_events) {
  for (auto& p : main_window_manager_->main_windows()) {
    auto& main_window = *p.second;
    bool events_shown =
        main_window.FindOpenedViewByType(kEventWindowInfo) != nullptr;
    if (has_events != events_shown) {
      if (has_events && profile_->event_auto_show)
        main_window.OpenPane(kEventWindowInfo, false);
      else if (!has_events && profile_->event_auto_hide)
        main_window.ClosePane(kEventWindowInfo);
    }

    main_window.SetWindowFlashing(has_events && profile_->event_flash_window);
  }
}

bool ClientApplication::Login() {
  assert(base::CommandLine::ForCurrentProcess());
  auto& command_line = *base::CommandLine::ForCurrentProcess();

  scada::ServiceLogParams service_log_params{
      command_line.HasSwitch("log-service-read"),
      command_line.HasSwitch("log-service-browse"),
      command_line.HasSwitch("log-service-history"),
      command_line.HasSwitch("log-service-event"),
      command_line.HasSwitch("log-service-model-change-event"),
      command_line.HasSwitch("log-service-node-semantics-change-event"),
  };

  DataServicesContext services_context{logger_, *io_context_,
                                       *transport_factory_, service_log_params};

  DataServices services;
  if (!ExecuteLoginDialog(std::move(services_context), services))
    return false;

  auto audit = std::make_shared<Audit>(
      *io_context_,
      std::make_shared<AuditLoggerImpl>(
          std::make_shared<NestedLogger>(logger_, "Audit")),
      std::move(services.attribute_service_),
      std::move(services.view_service_));

  services.attribute_service_ = audit;
  services.view_service_ = audit;

  master_data_services_->SetServices(std::move(services));

  return true;
}

void ClientApplication::Quit() {
  if (!master_data_services_) {
    quit_handler_();
    return;
  }

  local_events_->ReportEvent(LocalEvents::SEV_ERROR,
                             base::WideToUTF16(L"Отключение от сервера..."));

  master_data_services_->Disconnect(
      [this](const scada::Status& status) { quit_handler_(); });
}
