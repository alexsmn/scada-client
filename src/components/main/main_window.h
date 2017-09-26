#pragma once

#include <functional>

#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/strings/string16.h"
#include "components/main/main_window.h"
#include "components/main/view_manager_delegate.h"
#include "contents_observer.h"
#include "core/node_id.h"

namespace base {
class FilePath;
}

namespace events {
class EventManager;
}

namespace scada {
class HistoryService;
class MethodService;
class NodeManagementService;
class MonitoredItemService;
class SessionService;
}

class ActionManager;
class ClientApplication;
class ConnectionStateReporter;
class ContentsObserver;
class DialogService;
class EventsHelper;
class Favourites;
class FileCache;
class LocalEvents;
class NodeRefService;
class MainCommands;
class MainWindow;
class OpenedView;
class Page;
class PortfolioManager;
class Profile;
class Speech;
class TaskManager;
class TimedDataService;
class ViewManager;
class WindowDefinition;
struct MainWindowDef;

struct MainWindowContext {
  int window_id_;
  FileCache& file_cache_;
  TimedDataService& timed_data_service_;
  NodeRefService& node_service_;
  PortfolioManager& portfolio_manager_;
  TaskManager& task_manager_;
  ActionManager& action_manager_;
  Profile& profile_;
  LocalEvents& local_events_;
  events::EventManager& event_manager_;
  std::function<void(int window_id)> close_handler_;
  std::function<bool(int page_id)> page_opened_;
  std::function<Page*()> find_closed_page_;
  std::function<OpenedView*(const base::FilePath& path)> find_opened_view_;
  std::function<void()> new_main_window_;
  Speech& speech_;
  scada::NodeManagementService& node_management_service_;
  scada::HistoryService& history_service_;
  scada::MethodService& method_service_;
  Favourites& favourites_;
  scada::SessionService& session_service_;
  scada::MonitoredItemService& monitored_item_service_;
};

class MainWindow : protected MainWindowContext,
                   protected ViewManagerDelegate,
                   private ContentsObserver {
 public:
  explicit MainWindow(MainWindowContext&& context);
  virtual ~MainWindow();

  int window_id() const { return window_id_; }

  void Close();

  MainWindowDef& GetPrefs() const;

  // Pages.  
  const Page& current_page() const;

  OpenedView* active_view() const { return active_view_; }
  OpenedView* active_data_view() const { return active_data_view_; }
  
  void ActivateView(OpenedView& view);
  void CloseView(OpenedView& view);

  void OnViewTitleUpdated(OpenedView& view, const base::string16& title);

  void AddContentsObserver(ContentsObserver& observer);
  void RemoveContentsObserver(ContentsObserver& observer);

  base::WeakPtr<MainWindow> GetWeakPtr() { return weak_factory_.GetWeakPtr(); }

  // MainWindow
  const Page* GetCurrentPage() const { return &current_page(); }
  void OpenPage(const Page& page);
  void SavePage();
  void SetPageTitle(const base::string16& title);
  OpenedView* GetActiveView();
  OpenedView* GetActiveDataView();
  OpenedView* OpenView(const WindowDefinition& window_definition, bool make_active);
  OpenedView* FindOpenedViewByFilePath(const base::FilePath& path);
  OpenedView* FindOpenedViewByType(unsigned type_id);
  void OpenPane(unsigned type_id, bool activate);
  void ClosePane(unsigned type_id);

  virtual void SetWindowFlashing(bool flashing) = 0;
  virtual void UpdateToolbarPosition() = 0;

  virtual DialogService& GetDialogService() { return *(DialogService*)0; }

  ViewManager& view_manager() { assert(view_manager_); return *view_manager_; }
  const ViewManager& view_manager() const { assert(view_manager_); return *view_manager_; }

 protected:
  MainCommands& main_commands() { return *main_commands_; }

  void Init(ViewManager& view_manager);
  void BeforeClose();

  virtual void UpdateTitle() = 0;

  virtual void OnSelectionChanged() = 0;

  // ViewManagerDelegate
  virtual std::unique_ptr<OpenedView> OnCreateView(WindowDefinition& def) override;
  virtual void OnViewClosed(OpenedView& view, WindowDefinition& definition) override;
  virtual void OnActiveViewChanged(OpenedView* view) override;

 private:
  void SetActiveView(OpenedView* view);
  void SetActiveDataView(OpenedView* view);

  void OnEvents(bool has_events);

  // ContentsObserver
  virtual void OnContainedItemsUpdate(const std::set<scada::NodeId>& item_ids) override;
  virtual void OnContainedItemChanged(const scada::NodeId& item_id, bool added) override;

  OpenedView* FindViewToRecycle(unsigned type);

  OpenedView* active_view_ = nullptr;
  // View to insert new items.
  OpenedView* active_data_view_ = nullptr;

  // TODO: Move into application.
  std::unique_ptr<ConnectionStateReporter> connection_state_reporter_;
  
  std::unique_ptr<MainCommands> main_commands_;

  std::unique_ptr<EventsHelper> events_helper_;

  ViewManager* view_manager_ = nullptr;

  base::ObserverList<ContentsObserver> contents_observers_;

  base::WeakPtrFactory<MainWindow> weak_factory_;

  friend class OpenedView;
  friend class MainMenu;
  friend class MainMenuModel2;
  friend class NativeMainWindow;
  friend class MainCommands;
};