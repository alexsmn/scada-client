#pragma once

#include "base/observer_list.h"
#include "controller/contents_observer.h"
#include "main_window/main_window_context.h"
#include "main_window/main_window_interface.h"
#include "main_window/view_manager_delegate.h"

namespace aui {
class MenuModel;
}

class ContentsObserver;
class DialogService;
class MainCommands;
class OpenedView;
class Page;
class ViewManager;
class WindowDefinition;
struct MainWindowDef;
struct WindowInfo;

class MainWindow : protected MainWindowContext,
                   protected ViewManagerDelegate,
                   private ContentsObserver,
                   public MainWindowInterface {
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
  void SplitView(OpenedView& view, bool vertically);

  void OnViewTitleUpdated(OpenedView& view, const std::u16string& title);

  void AddContentsObserver(ContentsObserver& observer);
  void RemoveContentsObserver(ContentsObserver& observer);

  virtual void SetWindowFlashing(bool flashing) = 0;

  // MainWindow
  virtual const Page& GetCurrentPage() const override { return current_page(); }
  virtual void OpenPage(const Page& page) override;
  virtual void SetCurrentPageTitle(std::u16string_view title) override;
  virtual void SaveCurrentPage() override;
  virtual void DeleteCurrentPage() override;
  virtual OpenedView* GetActiveView() override;
  virtual OpenedView* GetActiveDataView() override;
  scada::status_promise<OpenedView*> OpenView(
      const WindowDefinition& window_definition,
      bool make_active);
  OpenedView* FindOpenedViewByFilePath(const std::filesystem::path& path);
  OpenedView* FindOpenedViewByType(const WindowInfo& window_info);
  void OpenPane(const WindowInfo& window_info, bool activate);
  void ClosePane(const WindowInfo& window_info);

  virtual DialogService& GetDialogService() = 0;

  CommandHandler& commands() { return *commands_; }

  void CleanupForTesting();

 protected:
  void Init(ViewManager& view_manager);
  void BeforeClose();

  virtual void OnSelectionChanged() = 0;

  virtual void UpdateTitle() = 0;
  virtual void SetToolbarPosition(unsigned position) = 0;

  virtual void ShowPopupMenu(aui::MenuModel* merge_menu,
                             unsigned resource_id,
                             const aui::Point& point,
                             bool right_click) = 0;

  // ViewManagerDelegate
  virtual std::unique_ptr<OpenedView> OnCreateView(
      WindowDefinition& def) override;
  virtual void OnViewClosed(OpenedView& view) override;
  virtual void OnActiveViewChanged(OpenedView* view) override;

  std::unique_ptr<CommandHandler> commands_;

  std::unique_ptr<aui::MenuModel> context_menu_model_;

  std::unique_ptr<aui::MenuModel> tab_popup_menu_;

 private:
  void SetActiveView(OpenedView* view);
  void SetActiveDataView(OpenedView* view);

  OpenedView* FindViewToRecycle(unsigned type);

  void ExecuteDefaultNodeCommand(const NodeRef& node);

  // ContentsObserver
  virtual void OnContentsChanged(
      const std::set<scada::NodeId>& item_ids) override;
  virtual void OnContainedItemChanged(const scada::NodeId& item_id,
                                      bool added) override;

  ViewManager* view_manager_ = nullptr;

  OpenedView* active_view_ = nullptr;
  // View to insert new items.
  OpenedView* active_data_view_ = nullptr;

  base::ObserverList<ContentsObserver> contents_observers_;

  friend class OpenedView;
  friend class NativeMainWindow;
  friend class MainCommands;
};
