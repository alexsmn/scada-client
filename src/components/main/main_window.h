#pragma once

#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "common/aliases.h"
#include "components/main/main_window_context.h"
#include "components/main/view_manager_delegate.h"
#include "contents_observer.h"

#include <functional>

namespace base {
class FilePath;
}

namespace events {
class EventManager;
}

namespace scada {
class HistoryService;
class MethodService;
class MonitoredItemService;
class NodeManagementService;
class SessionService;
class ViewService;
}  // namespace scada

class ActionManager;
class ClientApplication;
class ConnectionStateReporter;
class ContentsObserver;
class DialogService;
class EventsHelper;
class Favourites;
class FileCache;
class LocalEvents;
class MainCommands;
class MainWindow;
class NodeService;
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
  ActionManager& action_manager_;
  const AliasResolver alias_resolver_;
  const int window_id_;
  events::EventManager& event_manager_;
  Favourites& favourites_;
  FileCache& file_cache_;
  LocalEvents& local_events_;
  NodeService& node_service_;
  PortfolioManager& portfolio_manager_;
  Profile& profile_;
  scada::HistoryService& history_service_;
  scada::MethodService& method_service_;
  scada::MonitoredItemService& monitored_item_service_;
  scada::NodeManagementService& node_management_service_;
  scada::SessionService& session_service_;
  scada::ViewService& view_service_;
  Speech& speech_;
  std::function<bool(int page_id)> page_opened_;
  std::function<OpenedView*(const base::FilePath& path)> find_opened_view_;
  std::function<Page*()> find_closed_page_;
  std::function<void()> new_main_window_;
  std::function<void(int window_id)> close_handler_;
  TaskManager& task_manager_;
  TimedDataService& timed_data_service_;
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
  OpenedView* OpenView(const WindowDefinition& window_definition,
                       bool make_active);
  OpenedView* FindOpenedViewByFilePath(const base::FilePath& path);
  OpenedView* FindOpenedViewByType(unsigned type_id);
  void OpenPane(unsigned type_id, bool activate);
  void ClosePane(unsigned type_id);


  virtual DialogService& GetDialogService() { return *(DialogService*)0; }

  ViewManager& view_manager() {
    assert(view_manager_);
    return *view_manager_;
  }
  const ViewManager& view_manager() const {
    assert(view_manager_);
    return *view_manager_;
  }

 protected:
  MainCommands& main_commands() { return *main_commands_; }

  void Init(ViewManager& view_manager);
  void BeforeClose();

  virtual void SetWindowFlashing(bool flashing) = 0;

  virtual void OnSelectionChanged() = 0;

  virtual void UpdateTitle() = 0;
  virtual void UpdateToolbarPosition() = 0;

  // ViewManagerDelegate
  virtual std::unique_ptr<OpenedView> OnCreateView(
      WindowDefinition& def) override;
  virtual void OnViewClosed(OpenedView& view,
                            WindowDefinition& definition) override;
  virtual void OnActiveViewChanged(OpenedView* view) override;

  std::unique_ptr<MainCommands> main_commands_;

 private:
  void SetActiveView(OpenedView* view);
  void SetActiveDataView(OpenedView* view);

  OpenedView* FindViewToRecycle(unsigned type);

  void OnEvents(bool has_events);

  // ContentsObserver
  virtual void OnContainedItemsUpdate(
      const std::set<scada::NodeId>& item_ids) override;
  virtual void OnContainedItemChanged(const scada::NodeId& item_id,
                                      bool added) override;

  OpenedView* active_view_ = nullptr;
  // View to insert new items.
  OpenedView* active_data_view_ = nullptr;

  // TODO: Move into application.
  std::unique_ptr<ConnectionStateReporter> connection_state_reporter_;

  std::unique_ptr<EventsHelper> events_helper_;

  ViewManager* view_manager_ = nullptr;

  base::ObserverList<ContentsObserver> contents_observers_;

  base::WeakPtrFactory<MainWindow> weak_factory_{this};

  friend class OpenedView;
  friend class MainMenu;
  friend class MainMenuModel2;
  friend class NativeMainWindow;
  friend class MainCommands;
};
