#pragma once

#include "controller/node_id_set.h"
#include "main_window/main_window_context.h"
#include "main_window/main_window_interface.h"
#include "main_window/view_manager_delegate.h"

#include <boost/signals2/connection.hpp>
#include <boost/signals2/signal.hpp>
#include <filesystem>
#include <functional>

namespace scada::aui {
class MenuModel;
}

class DialogService;
class MainWindowCommandRouter;
class OpenedView;
class Page;
class ViewManager;
class WindowDefinition;
struct MainWindowDef;
struct WindowInfo;

class BaseMainWindow : protected MainWindowContext,
                       protected ViewManagerDelegate,
                       public MainWindowInterface {
 public:
  BaseMainWindow(MainWindowContext&& context, DialogService& dialog_service);
  virtual ~BaseMainWindow();

  int window_id() const { return window_id_; }

  void Close();

  MainWindowDef& GetPrefs() const;

  // Pages.

  const Page& current_page() const;

  // Views.

  const std::list<OpenedView*>& opened_views() const;
  std::vector<OpenedViewInterface*> GetOpenedViews() const override;

  OpenedView* active_view() const { return active_view_; }
  OpenedView* active_data_view() const { return active_data_view_; }

  void ActivateView(const OpenedViewInterface& view) override;
  void CloseView(OpenedView& view);
  void SplitView(OpenedViewInterface& view, bool vertically) override;

  void OnViewTitleUpdated(OpenedView& view, const std::u16string& title);

  using ContentsChangedCallback =
      std::function<void(const NodeIdSet& node_ids)>;
  using ContainedItemChangedCallback =
      std::function<void(const scada::NodeId& node_id, bool added)>;

  // Notifies after the contained-item set of the active data view changed
  // wholesale.
  [[nodiscard]] boost::signals2::scoped_connection SubscribeContentsChanged(
      const ContentsChangedCallback& callback);
  // Notifies after a single contained item was added or removed.
  [[nodiscard]] boost::signals2::scoped_connection
  SubscribeContainedItemChanged(const ContainedItemChangedCallback& callback);

  // MainWindow
  virtual int GetMainWindowId() const { return window_id(); }
  virtual const Page& GetCurrentPage() const override { return current_page(); }
  virtual void OpenPage(const Page& page) override;
  virtual void SetCurrentPageTitle(std::u16string_view title) override;
  virtual void SaveCurrentPage() override;
  virtual void DeleteCurrentPage() override;
  virtual OpenedView* GetActiveView() const override;
  virtual OpenedView* GetActiveDataView() const override;
  virtual Awaitable<OpenedViewInterface*> OpenView(
      const WindowDefinition& window_definition,
      bool activate = true) override;
  OpenedView* FindOpenedViewByFilePath(const std::filesystem::path& path);
  OpenedViewInterface* FindViewByType(
      std::string_view window_type) const override;
  void OpenPane(const WindowInfo& window_info, bool activate);
  void ClosePane(const WindowInfo& window_info);

  virtual DialogService& GetDialogService() = 0;

  virtual void SetWindowFlashing(bool flashing) = 0;

  CommandHandler& commands() { return *commands_; }

  void CleanupForTesting();
  bool IsContextMenuCommandAvailableForTesting(unsigned command_id);

  // TODO: Move to a separate class.
  virtual void ShowPopupMenu(scada::aui::MenuModel* merge_menu,
                             unsigned resource_id,
                             const scada::aui::Point& point,
                             bool right_click) = 0;

  // TODO: Move to a separate class.
  void ExecuteDefaultNodeCommand(const NodeRef& node);

  static void SetHideForTesting(bool hide = true) { g_hide_for_testing = hide; }

 protected:
  void AttachViewManager(ViewManager& view_manager);
  void Init(ViewManager& view_manager);
  void BeforeClose();

  virtual void OnSelectionChanged() = 0;

  virtual void UpdateTitle() = 0;
  virtual void SetToolbarPosition(unsigned position) = 0;

  // ViewManagerDelegate
  virtual void OnViewClosed(OpenedView& view) override;
  virtual void OnActiveViewChanged(OpenedView* view) override;

  std::unique_ptr<CommandHandler> commands_;

  std::unique_ptr<scada::aui::MenuModel> context_menu_model_;

  std::unique_ptr<scada::aui::MenuModel> tab_popup_menu_;

  static bool g_hide_for_testing;

 private:
  void SetActiveView(OpenedView* view);
  void SetActiveDataView(OpenedView* view);

  OpenedView* FindViewToRecycle(unsigned type);

  void OnContentsChanged(const std::set<scada::NodeId>& item_ids);
  void OnContainedItemChanged(const scada::NodeId& item_id, bool added);

  ViewManager* view_manager_ = nullptr;

  OpenedView* active_view_ = nullptr;
  // View to insert new items.
  OpenedView* active_data_view_ = nullptr;

  boost::signals2::signal<void(const NodeIdSet&)> contents_changed_signal_;
  boost::signals2::signal<void(const scada::NodeId&, bool)>
      contained_item_changed_signal_;

  friend class OpenedView;
  friend class NativeMainWindow;
  friend class MainWindowCommandRouter;
};
