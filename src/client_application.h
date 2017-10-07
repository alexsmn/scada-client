#pragma once

#include <boost/asio/io_service.hpp>
#include <map>
#include <memory>
#include <string>

#include "base/files/file_path.h"
#include "core/node_id.h"
#include "core/session_state_observer.h"
#include "core/data_services.h"
#include "core/data_services_factory.h"

namespace events {
class EventManager;
}

namespace net {
class TransportFactory;
}

class ActionManager;
class Favourites;
class FileCache;
class Logger;
class LocalEvents;
class MainWindow;
class MasterDataServices;
class ModusModule2;
class NodeRefService;
class OpenedView;
class Page;
class PortfolioManager;
class Profile;
class TaskManager;
class TimedDataService;
class Speech;
struct MainWindowContext;

class ClientApplication : private scada::SessionStateObserver {
 public:
  ClientApplication(int argc, char** argv);
  virtual ~ClientApplication();

  ClientApplication(const ClientApplication&) = delete;
  ClientApplication& operator=(const ClientApplication&) = delete;

  Logger& logger() const { return *logger_; }
  
  scada::SessionService& session_service();

  virtual bool Init();
  bool ShowLoginDialog();
  void BeforeRun();
  virtual int Run(int show) = 0;
  virtual void Quit() = 0;

  void NewMainWindow();
  void OpenMainWindow(int window_id);
  void CloseMainWindow(int window_id);

  typedef std::map<int /*window_id*/, std::unique_ptr<MainWindow>> MainWindows;
  const MainWindows& main_windows() const { return main_windows_; }

 protected:
  virtual std::unique_ptr<MainWindow> CreateMainWindow(MainWindowContext&& context) = 0;

  virtual bool ShowLoginDialogImpl(const DataServicesContext& context, DataServices& services) = 0;

  std::shared_ptr<Logger> logger_;

  std::shared_ptr<MasterDataServices> master_data_services_;

 private:
  bool IsPageOpened(int page_id);
  Page* FindFirstNotOpenedPage();
  OpenedView* FindOpenedViewByFilePath(const base::FilePath& path);

  // scada::SessionStateObserver
  virtual void OnSessionCreated() override;
  virtual void OnSessionDeleted(const scada::Status& status) override;

  boost::asio::io_service io_service_;
  std::unique_ptr<net::TransportFactory> transport_factory_;

  std::unique_ptr<NodeRefService> node_service_;

  std::unique_ptr<events::EventManager> event_manager_;
  std::unique_ptr<TimedDataService> timed_data_service_;

  std::unique_ptr<LocalEvents> local_events_;
  std::unique_ptr<TaskManager> task_manager_;
  std::unique_ptr<PortfolioManager> portfolio_manager_;
  std::unique_ptr<Speech> speech_;

  std::unique_ptr<FileCache> file_cache_;

  std::unique_ptr<ModusModule2> modus_module_;

  std::unique_ptr<ActionManager> action_manager_;

  std::unique_ptr<Profile> profile_;

  std::unique_ptr<Favourites> favourites_;

  MainWindows main_windows_;

  bool profile_loaded_ = false;
};
