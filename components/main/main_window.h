#pragma once

#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "common/aliases.h"
#include "components/main/action.h"
#include "components/main/main_window_context.h"
#include "components/main/view_manager_delegate.h"
#include "contents_observer.h"

#include <map>

namespace base {
class FilePath;
}

class ConnectionStateReporter;
class ContentsObserver;
class DialogService;
class EventsHelper;
class MainCommands;
class MainMenuModel;
class OpenedView;
class Page;
class PageLayoutBlock;
class ViewManager;
class WindowDefinition;
struct MainWindowDef;

class MainWindow : protected MainWindowContext,
                   protected ViewManagerDelegate,
                   private ContentsObserver {
 public:
  MainWindow(MainWindowContext&& context, DialogService& dialog_service);
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

 protected:
  void Init(ViewManager& view_manager);
  void BeforeClose();

  virtual DialogService& GetDialogService() = 0;

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

  ViewManager* view_manager_ = nullptr;

  OpenedView* active_view_ = nullptr;
  // View to insert new items.
  OpenedView* active_data_view_ = nullptr;

  // TODO: Move into application.
  std::unique_ptr<ConnectionStateReporter> connection_state_reporter_;

  std::unique_ptr<EventsHelper> events_helper_;

  base::ObserverList<ContentsObserver> contents_observers_;

  base::WeakPtrFactory<MainWindow> weak_factory_{this};

  friend class OpenedView;
  friend class MainMenu;
  friend class MainMenuModel2;
  friend class NativeMainWindow;
  friend class MainCommands;
};
