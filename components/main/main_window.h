#pragma once

#include "base/files/file_path.h"
#include "base/observer_list.h"
#include "common/aliases.h"
#include "components/main/action_manager.h"
#include "components/main/main_window_context.h"
#include "components/main/view_manager_delegate.h"
#include "contents_observer.h"

namespace base {
class FilePath;
}

class ContentsObserver;
class DialogService;
class MainCommands;
class MainMenuModel;
class OpenedView;
class Page;
class ViewManager;
class WindowDefinition;
struct MainWindowDef;
struct WindowInfo;

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
  void SplitView(OpenedView& view, bool vertically);

  void OnViewTitleUpdated(OpenedView& view, const std::u16string& title);

  void AddContentsObserver(ContentsObserver& observer);
  void RemoveContentsObserver(ContentsObserver& observer);

  virtual void SetWindowFlashing(bool flashing) = 0;

  // MainWindow
  const Page* GetCurrentPage() const { return &current_page(); }
  void OpenPage(const Page& page);
  void SavePage();
  void SetPageTitle(const std::u16string& title);
  OpenedView* GetActiveView();
  OpenedView* GetActiveDataView();
  OpenedView* OpenView(const WindowDefinition& window_definition,
                       bool make_active);
  OpenedView* FindOpenedViewByFilePath(const std::filesystem::path& path);
  OpenedView* FindOpenedViewByType(const WindowInfo& window_info);
  void OpenPane(const WindowInfo& window_info, bool activate);
  void ClosePane(const WindowInfo& window_info);

  virtual DialogService& GetDialogService() = 0;

  CommandHandler& commands() { return *commands_; }

 protected:
  void Init(ViewManager& view_manager);
  void BeforeClose();

  virtual void OnSelectionChanged() = 0;

  virtual void UpdateTitle() = 0;
  virtual void SetToolbarPosition(unsigned position) = 0;

  virtual void ShowPopupMenu(unsigned resource_id,
                             const aui::Point& point,
                             bool right_click) = 0;

  // ViewManagerDelegate
  virtual std::unique_ptr<OpenedView> OnCreateView(
      WindowDefinition& def) override;
  virtual void OnViewClosed(OpenedView& view) override;
  virtual void OnActiveViewChanged(OpenedView* view) override;

  std::unique_ptr<CommandHandler> commands_;

  std::unique_ptr<ui::MenuModel> context_menu_model_;

  std::unique_ptr<ui::MenuModel> tab_popup_menu_;

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
