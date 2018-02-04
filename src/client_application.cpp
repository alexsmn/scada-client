#include "client_application.h"

#include "base/command_line.h"
#include "base/logger.h"
#include "base/nested_logger.h"
#include "base/path_service.h"
#include "base/strings/string_util.h"
#include "base/win/dump.h"
#include "client_paths.h"
#include "client_utils.h"
#include "common/common_paths.h"
#include "common/event_manager.h"
#include "common/master_data_services.h"
#include "common/node_service_impl.h"
#include "common_resources.h"
#include "components/main/action.h"
#include "components/main/main_window.h"
#include "components/modus/modus_module2.h"
#include "components/vidicon_display/vidicon_client.h"
#include "core/session_service.h"
#include "net/session_info.h"
#include "net/transport_factory_impl.h"
#include "project.h"
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
REGISTER_DATA_SERVICES("Vidicon", L"Видикон", CreateVidiconServices);
REGISTER_DATA_SERVICES("OpcUa", L"OPC UA", CreateOpcUaServices);

ClientApplication* g_application = nullptr;

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

}  // namespace

ClientApplication::ClientApplication(int argc, char** argv)
    : master_data_services_{std::make_shared<MasterDataServices>()} {}

bool ClientApplication::Init() {
  if (!base::CommandLine::Init(0, nullptr)) {
    LOG(FATAL) << "Can't parse command line.";
    return false;
  }

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

  transport_factory_ = std::make_unique<net::TransportFactoryImpl>(io_service_);

  return true;
}

ClientApplication::~ClientApplication() {
  main_windows_.clear();

  if (profile_ && profile_loaded_)
    profile_->Save(*portfolio_manager_, *event_manager_, *favourites_);

  VidiconClient::CleanupInstance();

  // Shutdown OPC.
  // extern void ShutdownOpc();
  // ShutdownOpc();

  action_manager_.reset();

  ModusModule2::SetInstance(nullptr);
  modus_module_.reset();

  file_cache_.reset();

  speech_.reset();
  portfolio_manager_.reset();
  task_manager_.reset();
  profile_.reset();
  local_events_.reset();

  timed_data_service_.reset();
  event_manager_.reset();
  node_service_.reset();

  // Remove SessionService observer.
  master_data_services_->RemoveObserver(*this);
}

bool ClientApplication::ShowLoginDialog() {
  DataServicesContext context{
      logger_,
      *transport_factory_,
  };

  DataServices services;
  if (!ShowLoginDialogImpl(std::move(context), services))
    return false;

  master_data_services_->SetServices(std::move(services));
  return true;
}

void ClientApplication::BeforeRun() {
  // Add SessionService observer.
  master_data_services_->AddObserver(*this);

  node_service_ = std::make_unique<NodeServiceImpl>(NodeServiceImplContext{
      logger_,
      *master_data_services_,
      *master_data_services_,
  });

  event_manager_ =
      std::make_unique<events::EventManager>(events::EventManagerContext{
          io_service_,
          *master_data_services_,
          *master_data_services_,
          *master_data_services_,
          std::make_shared<NestedLogger>(logger_, "EventManager"),
      });

  timed_data_service_ = std::make_unique<TimedDataServiceImpl>(TimedDataContext{
      io_service_,
      *node_service_,
      *master_data_services_,
      *master_data_services_,
      *master_data_services_,
      *master_data_services_,
      *master_data_services_,
      *event_manager_,
  });

  local_events_ = std::make_unique<LocalEvents>();

  profile_ = std::make_unique<Profile>();

  task_manager_ = std::make_unique<TaskManagerImpl>(TaskManagerImplContext{
      *node_service_,
      *master_data_services_,
      *local_events_,
      *profile_,
  });
  speech_.reset(new Speech);

  file_cache_.reset(new FileCache);
  RegisterFileCacheType(*file_cache_, VIEW_TYPE_MODUS, L".sde;.xsde");
  RegisterFileCacheType(*file_cache_, VIEW_TYPE_VIDICON_DISPLAY, L".vds");
  RegisterFileCacheType(*file_cache_, VIEW_TYPE_EXCEL_REPORT, L".tsr");
  file_cache_->Init();

  modus_module_.reset(new ModusModule2);
  ModusModule2::SetInstance(modus_module_.get());

  favourites_ = std::make_unique<Favourites>();

  portfolio_manager_ = std::make_unique<PortfolioManager>(*node_service_);

  profile_->Load(*portfolio_manager_, *event_manager_, *favourites_);
  profile_loaded_ = true;

  typedef Profile::MainWindows Windows;
  const Windows& window_defs = profile_->main_windows;
  if (window_defs.empty()) {
    auto window_id = profile_->CreateWindowId();
    OpenMainWindow(window_id);
  } else {
    for (auto& p : window_defs)
      OpenMainWindow(p.second.id);
  }
}

void ClientApplication::NewMainWindow() {
  int window_id = profile_->CreateWindowId();
  OpenMainWindow(window_id);
}

void ClientApplication::OpenMainWindow(int window_id) {
  if (main_windows_.find(window_id) != main_windows_.end())
    return;

  if (!action_manager_)
    action_manager_ = std::make_unique<ActionManager>(*node_service_);

  auto window = CreateMainWindow(MainWindowContext{
      window_id,
      *file_cache_,
      *timed_data_service_,
      *node_service_,
      *portfolio_manager_,
      *task_manager_,
      *action_manager_,
      *profile_,
      *local_events_,
      *event_manager_,
      [this](int window_id) { CloseMainWindow(window_id); },
      [this](int page_id) { return IsPageOpened(page_id); },
      [this] { return FindFirstNotOpenedPage(); },
      [this](const base::FilePath& path) {
        return FindOpenedViewByFilePath(path);
      },
      [this] { NewMainWindow(); },
      *speech_,
      *master_data_services_,
      *master_data_services_,
      *master_data_services_,
      *favourites_,
      *master_data_services_,
      *master_data_services_,
      *master_data_services_,
  });

  if (window)
    main_windows_[window_id] = std::move(window);
}

void ClientApplication::CloseMainWindow(int window_id) {
  main_windows_.erase(window_id);

  if (main_windows_.empty())
    Quit();
  else
    profile_->main_windows.erase(window_id);
}

void ClientApplication::OnSessionCreated() {
  event_manager_->OnChannelOpened(master_data_services_->GetUserId());
}

void ClientApplication::OnSessionDeleted(const scada::Status& status) {
  event_manager_->OnChannelClosed();
}

bool ClientApplication::IsPageOpened(int page_id) {
  for (auto& p : main_windows_) {
    auto* page = p.second->GetCurrentPage();
    if (page && page->id == page_id)
      return true;
  }
  return false;
}

Page* ClientApplication::FindFirstNotOpenedPage() {
  for (auto& p : profile_->pages()) {
    if (!IsPageOpened(p.second.id))
      return &p.second;
  }
  return NULL;
}

OpenedView* ClientApplication::FindOpenedViewByFilePath(
    const base::FilePath& path) {
  for (auto& p : main_windows_) {
    auto view = p.second->FindOpenedViewByFilePath(path);
    if (view)
      return view;
  }
  return NULL;
}

scada::SessionService& ClientApplication::session_service() {
  return *master_data_services_;
}
