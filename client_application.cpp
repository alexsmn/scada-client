#include "client_application.h"

#include "address_space/address_space_impl.h"
#include "address_space/generic_node_factory.h"
#include "address_space/scada_address_space.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/nested_logger.h"
#include "base/path_service.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/win/dump.h"
#include "client_paths.h"
#include "common/address_space/address_space_fetcher.h"
#include "common/address_space/address_space_node_service.h"
#include "common/common_paths.h"
#include "common/event_manager.h"
#include "common/master_data_services.h"
#include "common/remote_node_service.h"
#include "components/login/login_dialog.h"
#include "components/main/action_manager.h"
#include "components/main/actions.h"
#include "components/main/context_menu_model.h"
#include "components/main/main_commands.h"
#include "components/main/main_menu_model.h"
#include "components/main/main_window.h"
#include "components/main/main_window_manager.h"
#include "components/main/opened_view_commands.h"
#include "components/main/status_bar_model_impl.h"
#include "components/modus/libmodus/modus_module2.h"
#include "components/vidicon_display/vidicon_client.h"
#include "net/transport_factory_impl.h"
#include "project.h"
#include "remote/session_proxy_notifier.h"
#include "services/alias_service.h"
#include "services/connection_state_reporter.h"
#include "services/event_notifier.h"
#include "services/favourites.h"
#include "services/file_cache.h"
#include "services/local_events.h"
#include "services/portfolio_manager.h"
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
#endif
// REGISTER_DATA_SERVICES("OpcUa", L"OPC UA", CreateOpcUaServices);

namespace {

LONG WINAPI ProcessUnhandledException(_EXCEPTION_POINTERS* exception) {
  SYSTEMTIME time;
  GetLocalTime(&time);

  base::FilePath path;
  PathService::Get(base::DIR_EXE, &path);
  path = path.Append(L"logs");
  base::CreateDirectory(path);

  // TODO: Take module name.
  std::wstring name = base::StringPrintf(
      L"client_%04d%02d%02d_%02d%02d%02d.dmp", time.wYear, time.wMonth,
      time.wDay, time.wHour, time.wMinute, time.wSecond);
  path = path.Append(name);

  DumpException(path.value().c_str(), *exception);

  return EXCEPTION_EXECUTE_HANDLER;
}

void RegisterFileCacheType(FileCache& cache,
                           ViewType type,
                           const base::string16& ext) {
  base::string16 name = base::SysNativeMBToWide(ViewTypeToString(type));
  cache.RegisterType(type, name, ext);
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
    base::FilePath path;
    PathService::Get(client::DIR_LOG, &path);
    base::CreateDirectory(path);
    path = path.Append(FILE_PATH_LITERAL("client.log"));
    logging::LoggingSettings log;
    log.logging_dest = logging::LOG_TO_FILE;
    log.log_file = path.value().c_str();
    log.lock_log = logging::LOCK_LOG_FILE;
    log.delete_old = logging::APPEND_TO_OLD_LOG_FILE;
    logging::InitLogging(log);
  }

  SetUnhandledExceptionFilter(ProcessUnhandledException);

  logger_ = CreateFileLogger(
      client::DIR_LOG, L"client",
      "Telecontrol SCADA Client " PROJECT_VERSION_DOTTED_STRING);

  io_context_ = std::make_unique<boost::asio::io_context>();
  io_context_timer_ = std::make_unique<base::Timer>(true, true);
  io_context_timer_->Start(
      FROM_HERE, base::TimeDelta::FromMilliseconds(10),
      base::Bind(&PollIoContext, base::Unretained(io_context_.get())));

  transport_factory_ =
      std::make_unique<net::TransportFactoryImpl>(*io_context_);
}

ClientApplication::~ClientApplication() {
  main_window_manager_.reset();

  if (profile_ && profile_loaded_)
    profile_->Save(*event_manager_, *portfolio_manager_, *favourites_);

  VidiconClient::CleanupInstance();

  // Shutdown OPC.
  // extern void ShutdownOpc();
  // ShutdownOpc();

  ModusModule2::SetInstance(nullptr);
  modus_module_.reset();

  file_cache_.reset();
  connection_state_reporter_.reset();

  speech_.reset();
  action_manager_.reset();
  favourites_.reset();
  portfolio_manager_.reset();
  task_manager_.reset();
  event_notifier_.reset();
  local_events_.reset();

  profile_.reset();

  timed_data_service_.reset();
  alias_resolver_ = nullptr;
  event_manager_.reset();
  node_service_.reset();

  master_data_services_->RemoveObserver(*this);
  master_data_services_ = nullptr;

  transport_factory_.reset();
  io_context_timer_.reset();
  io_context_.reset();
}

void ClientApplication::Start() {
  master_data_services_->AddObserver(*this);

  event_manager_ =
      std::make_unique<events::EventManager>(events::EventManagerContext{
          *io_context_,
          *master_data_services_,
          *master_data_services_,
          *master_data_services_,
          std::make_shared<NestedLogger>(logger_, "EventManager"),
      });
  if (master_data_services_->IsConnected())
    event_manager_->OnChannelOpened(master_data_services_->GetUserId());

  node_service_ =
      base::CommandLine::ForCurrentProcess()->HasSwitch("new-node-service")
          ? CreateRemoteNodeService()
          : CreateAddressSpaceNodeService();

  auto alias_logger =
      base::CommandLine::ForCurrentProcess()->HasSwitch("verbose-logging")
          ? static_cast<std::shared_ptr<Logger>>(
                std::make_shared<NestedLogger>(logger_, "AliasService"))
          : static_cast<std::shared_ptr<Logger>>(
                std::make_shared<NullLogger>());
  auto alias_service = std::make_shared<AliasService>(AliasServiceContext{
      alias_logger,
      *node_service_,
  });
  alias_resolver_ = [alias_service](base::StringPiece alias,
                                    const AliasResolveCallback& callback) {
    alias_service->Resolve(alias, callback);
  };

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
          *event_manager_,
      },
      std::make_shared<NestedLogger>(logger_, "TimedDataService"));

  profile_ = std::make_unique<Profile>();
  local_events_ = std::make_unique<LocalEvents>();

  event_notifier_ = std::make_unique<EventNotifier>(
      EventNotifierContext{*event_manager_, *local_events_, *profile_,
                           [this](bool has_events) { OnEvents(has_events); }});

  task_manager_ = std::make_unique<TaskManagerImpl>(TaskManagerImplContext{
      *node_service_, *master_data_services_, *local_events_, *profile_});
  speech_.reset(new Speech);

  connection_state_reporter_ = std::make_unique<ConnectionStateReporter>(
      ConnectionStateReporterContext{*master_data_services_, *local_events_});

  file_cache_ = std::make_unique<FileCache>();
  RegisterFileCacheType(*file_cache_, VIEW_TYPE_MODUS, L".sde;.xsde");
  RegisterFileCacheType(*file_cache_, VIEW_TYPE_VIDICON_DISPLAY, L".vds");
  RegisterFileCacheType(*file_cache_, VIEW_TYPE_EXCEL_REPORT, L".tsr");
  file_cache_->Init();

  modus_module_.reset(new ModusModule2);
  ModusModule2::SetInstance(modus_module_.get());

  action_manager_ = std::make_unique<ActionManager>();
  AddGlobalActions(*action_manager_, *node_service_);

  favourites_ = std::make_unique<Favourites>();

  portfolio_manager_ = std::make_unique<PortfolioManager>(
      PortfolioManagerContext{*node_service_});

  profile_->Load(*event_manager_, *portfolio_manager_, *favourites_);
  profile_loaded_ = true;

  main_window_manager_ =
      std::make_unique<MainWindowManager>(MainWindowManagerContext{
          *profile_,
          [this](int window_id) {
            return main_window_factory_(MakeMainWindowContext(window_id));
          },
          quit_handler_});

  // |main_window_manager_| must be assigned.
  main_window_manager_->Init();
}

std::shared_ptr<NodeService> ClientApplication::CreateRemoteNodeService() {
  struct Context {
    Context(const std::shared_ptr<Logger>& logger, MasterDataServices& services)
        : node_service{RemoteNodeServiceContext{
              std::make_shared<NestedLogger>(logger, "RemoteNodeService"),
              services, services}},
          node_service_notifier{node_service, services} {}

    RemoteNodeService node_service;
    SessionProxyNotifier<RemoteNodeService> node_service_notifier;
  };

  auto context = std::make_shared<Context>(logger_, *master_data_services_);
  return std::shared_ptr<NodeService>{context, &context->node_service};
}

std::shared_ptr<NodeService>
ClientApplication::CreateAddressSpaceNodeService() {
  class ClientAddressSpace : public AddressSpaceImpl {
   public:
    explicit ClientAddressSpace(const std::shared_ptr<Logger>& logger)
        : AddressSpaceImpl{logger}, node_factory{logger, *this} {
      CreateScadaAddressSpace(*this, node_factory);
    }

    GenericNodeFactory node_factory;
  };

  struct Context {
    Context(std::shared_ptr<Logger> input_logger, MasterDataServices& services)
        : logger{std::move(input_logger)},
          services{services},
          address_space{std::make_shared<NestedLogger>(logger, "AddressSpace")},
          node_service{MakeAddressSpaceNodeServiceContext()},
          node_service_notifier{node_service, services} {}

    AddressSpaceNodeServiceContext MakeAddressSpaceNodeServiceContext() {
      return {std::make_shared<NestedLogger>(logger, "NodeService"), services,
              services, address_space, address_space.node_factory};
    }

    const std::shared_ptr<Logger> logger;
    MasterDataServices& services;
    ClientAddressSpace address_space;
    AddressSpaceNodeService node_service;
    SessionProxyNotifier<AddressSpaceNodeService> node_service_notifier;
  };

  auto logger =
      base::CommandLine::ForCurrentProcess()->HasSwitch("verbose-logging")
          ? logger_
          : static_cast<std::shared_ptr<Logger>>(
                std::make_shared<NullLogger>());

  auto context = std::make_shared<Context>(logger, *master_data_services_);
  return std::shared_ptr<NodeService>{context, &context->node_service};
}

MainWindowContext ClientApplication::MakeMainWindowContext(int window_id) {
  auto controller_factory = [this](unsigned type, ControllerDelegate& delegate,
                                   DialogService& dialog_service) {
    return CreateController(
        type,
        ControllerContext{delegate, alias_resolver_, *task_manager_,
                          *master_data_services_, *event_manager_,
                          *master_data_services_, *master_data_services_,
                          *timed_data_service_, *node_service_,
                          *portfolio_manager_, *local_events_, *favourites_,
                          *file_cache_, *profile_, dialog_service});
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
        *event_manager_, *node_service_, *local_events_, *favourites_, *speech_,
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

  auto view_commands_factory = [this](OpenedView& opened_view,
                                      DialogService& dialog_service) {
    auto commands =
        std::make_unique<OpenedViewCommands>(OpenedViewCommandsContext{
            *task_manager_, *master_data_services_, *master_data_services_,
            *master_data_services_, *event_manager_, *master_data_services_,
            *master_data_services_, *timed_data_service_, *node_service_,
            *portfolio_manager_, *action_manager_, *local_events_, *favourites_,
            *file_cache_, *profile_, *main_window_manager_});

    commands->SetContext(&opened_view, &dialog_service);

    return commands;
  };

  auto status_bar_model =
      std::make_shared<StatusBarModelImpl>(StatusBarModelImplContext{
          *master_data_services_, *event_manager_, *node_service_});

  auto connection_info_provider = [this] {
    return base::SysNativeMBToWide(master_data_services_->GetHostName());
  };

  return MainWindowContext{*action_manager_,
                           window_id,
                           *file_cache_,
                           *main_window_manager_,
                           *profile_,
                           controller_factory,
                           main_commands_factory,
                           view_commands_factory,
                           std::move(status_bar_model),
                           context_menu_factory,
                           main_menu_factory,
                           connection_info_provider};
}

void ClientApplication::OnSessionCreated() {
  event_manager_->OnChannelOpened(master_data_services_->GetUserId());
}

void ClientApplication::OnSessionDeleted(const scada::Status& status) {
  event_manager_->OnChannelClosed();
}

void ClientApplication::OnEvents(bool has_events) {
  for (auto& p : main_window_manager_->main_windows()) {
    auto& main_window = *p.second;
    bool events_shown =
        main_window.FindOpenedViewByType(ID_EVENT_VIEW) != nullptr;
    if (has_events != events_shown) {
      if (has_events && profile_->event_auto_show)
        main_window.OpenPane(ID_EVENT_VIEW, false);
      else if (!has_events && profile_->event_auto_hide)
        main_window.ClosePane(ID_EVENT_VIEW);
    }

    main_window.SetWindowFlashing(has_events && profile_->event_flash_window);
  }
}

bool ClientApplication::Login() {
  DataServicesContext services_context{
      logger_, base::ThreadTaskRunnerHandle::Get(), *transport_factory_};
  DataServices services;
  if (!ExecuteLoginDialog(std::move(services_context), services))
    return false;

  master_data_services_->SetServices(std::move(services));
  return true;
}
