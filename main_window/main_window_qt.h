#pragma once

#include "aui/qt/dialog_service_impl_qt.h"
#include "controller/action_manager.h"
#include "controller/command_ui_registry.h"
#include "main_window/base_main_window.h"
#include "main_window/pages/page_switcher.h"
#include "main_window/pane_modes.h"

#include <QMainWindow>

#include <boost/signals2/connection.hpp>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace scada {
class NodeId;
}

class ActivityBar;
class CommandActions;
class Breadcrumb;
class CommandField;
class PageSwitcher;
class PaneModeController;
class InspectorPanel;
struct InspectorOpenAction;
struct InspectorSeriesView;
class SeriesModel;
class SelectionPanel;
class TagSearchIndex;
class QAction;
class QDockWidget;
class QLabel;
class QMenu;
class QPoint;
class QToolBar;
class QWidget;
class ProgressController;
class SettingsPanel;
class ViewManager;

class MainWindow final : public QMainWindow, public BaseMainWindow {
  Q_OBJECT

 public:
  explicit MainWindow(MainWindowContext&& context);
  ~MainWindow();

  // Switches the left sidebar to `mode`: closes the panes that do not belong
  // to it, opens the ones that do, fronts the first, and records the choice in
  // the window's profile preferences. Public because selecting a pane's mode
  // is how any caller — the rail, or the screenshot generator capturing that
  // pane — brings it on screen.
  void SetPaneMode(PaneModeId mode);
  // Selects the mode that owns `window_type`, if any. Returns false when no
  // mode claims it (a workspace view, or the bottom-docked Events pane).
  bool SelectPaneModeForPane(std::string_view window_type);

  // BaseMainWindow
  virtual DialogService& GetDialogService() override { return dialog_service_; }
  virtual void SetWindowFlashing(bool flashing) override;
  // Whether the window is currently asking for the operator's attention.
  // Qt owns the alert itself and exposes no way to read it back — the platform
  // alert state lives behind QPlatformWindow — so the request is mirrored here.
  // It is the requested state, not the state of the taskbar entry: the platform
  // drops the alert on activation without telling us.
  bool IsWindowFlashing() const { return window_flashing_; }
  virtual void ShowPopupMenu(scada::aui::MenuModel* merge_menu,
                             const scada::aui::Point& point,
                             bool right_click) override;

  // Receives a populated context menu in place of it being popped up.
  using PopupMenuInterceptor = std::function<void(QMenu&)>;

  // Diverts `ShowPopupMenu` to `interceptor`, which is handed the menu after
  // it has been built and before `exec()` would run. Pass an empty function to
  // restore the normal behaviour.
  //
  // This exists for the screenshot generator's `auto-menu` context captures,
  // and the diversion point is deliberate: the menu is built by the one code
  // path the operator's right-click uses, so the published image cannot
  // document a menu nobody sees. Nothing before this point is bypassed either,
  // which is what makes the capture faithful — the view that was clicked
  // supplies its own `merge_menu` (a grid's command set differs from the
  // object tree's), and `BuildMenu` calls `MenuWillShow()` so each row's
  // enabled state is resolved against the live selection. Several of the
  // manual's images show disabled rows, and a menu assembled some other way
  // would render them all enabled.
  //
  // A capture cannot simply call `ShowPopupMenu` and grab the result: Qt's
  // `QMenu::exec` runs a nested event loop and does not return until something
  // dismisses the menu, and offscreen nothing does.
  void SetPopupMenuInterceptor(PopupMenuInterceptor interceptor);

 protected:
  // BaseMainWindow
  virtual void UpdateTitle() override;
  virtual void OnSelectionChanged() override;
  virtual void SetToolbarPosition(unsigned position) override;
  virtual std::unique_ptr<OpenedView> OnCreateView(
      WindowDefinition& def) override;

  // ViewManagerDelegate
  // The tab strip's `+`: a "New view for <subject>" group over the views the
  // selection accepts, then an "Empty" group of the ones that open with no
  // selection at all (docs/product/ui-mockups/screens/shell-chrome.html).
  virtual void OnShowNewViewMenu(const scada::aui::Point& point) override;
  virtual void OnShowTabPopupMenu(OpenedView& view,
                                  const scada::aui::Point& point) override;
  // Both re-derive the rail marker: closing a pane by hand or activating a
  // different one changes which mode the sidebar is actually showing.
  virtual void OnViewClosed(OpenedView& view) override;
  virtual void OnActiveViewChanged(OpenedView* view) override;

  // BaseMainWindow
  virtual void OpenPage(const Page& page) override;

  // QWidget
  virtual void closeEvent(QCloseEvent* event) override;

 private:
  // Tabs the specialist panels onto the Inspector dock and fronts the
  // Inspector. Re-applied after every page open, because opening a page
  // restores a persisted QMainWindow dock state that includes these docks and
  // would otherwise leave them stacked vertically.
  void TabifySpecialistDocks();
  void CreateMenuBar();
  // Builds the grip command toolbar. It is off by default and stays that way
  // unless the operator turns the `Toolbar` preference on: shell.md §2.2 gives
  // that role to the top context bar ("Replaces the grip toolbar"), and the two
  // shown together draw three stacked rows of chrome — menu bar, context bar
  // and grip toolbar — where every mockup screen draws one. Every command it
  // carries stays reachable from the menu bar, the node context menu and the
  // Ctrl-K palette, so leaving it off hides a duplicate surface rather than a
  // capability. It lays out `command_actions_` and creates none of them.
  void CreateToolbar();
  void CreateStatusBar();
  // The top context bar: the command/search field and alarm state.
  void CreateContextBar();
  // Re-derives the context bar's breadcrumb: page → active view → selected
  // object. Cheap and idempotent, so it is called from every hook that can move
  // any of the three rather than trying to work out which one moved.
  void RefreshBreadcrumb();
  // The left activity rail (backlog 1.1): selects which panes occupy the
  // left sidebar. It never opens a workspace tab and never switches the page.
  void CreateActivityBar();
  // The right Inspector dock (backlog 2.6): reflects the active view's
  // selection — identity, live value, control action.
  // Why the selection-scoped control command is unavailable, for the
  // Inspector's disabled Control button. Empty when nothing is selected.
  QString ControlUnavailableReason();
  void CreateInspectorPanel();
  // The active view's plotted-series model, or null for a view that plots
  // nothing (every view but the chart). Resolved per call rather than cached:
  // a view can close between a click and its handler.
  SeriesModel* ActiveSeriesModel();
  // That model read out for the Inspector's series section, or nullopt when
  // there is no series to describe.
  std::optional<InspectorSeriesView> ActiveSeriesView();
  // The CATEGORY_OPEN selection commands the current selection accepts, in
  // registration order, for the Inspector's Open section. Availability is not
  // uniform — two of the seven additionally need a connected item — so a short
  // list is the healthy case, and an empty one (nothing selected) hides the
  // section. See docs/product/ui-mockups/authoring.md 4b "Opening a view".
  std::vector<InspectorOpenAction> OpenViewActions();
  // The current selection's display title, for a surface that has to name its
  // subject. Empty when nothing is selected.
  QString SelectionSubjectTitle();
  // The menu contributions that open a view with no selection -- the "Empty"
  // group of the tab strip's `+`. Derived from the registry rather than listed:
  // a contribution under the Graph or Table menu whose command id is a
  // registered WINDOW id is one that opens a view, which is what makes it an
  // empty-view opener. `Group Table` sits in the same menu and is not one,
  // because it is a selection command over the parent group.
  std::vector<MenuContribution> EmptyViewCommands();
  // Builds one dock per panel the feature modules registered, tabified onto
  // the Inspector in registration order.
  void CreateSelectionPanels();
  // Wires the rail's pages group: the page buttons, the "+" that creates one,
  // and the per-page context menu.
  void WireRailPages();
  // Rebuilds the rail's page buttons and re-marks the open page.
  void RefreshRailPages();
  // Rename / Delete for `page_id`, plus New — the same registered ID_PAGE_*
  // commands the Page menu uses.
  void ShowPageContextMenu(int page_id, const QPoint& global_pos);

  // The rail icon currently set on `page_id`, or empty when it has none. Read
  // back from the profile rather than cached, so the context menu's check mark
  // cannot disagree with what the rail draws.
  std::string PageIconFor(int page_id) const;
  // Sets `page_id`'s rail icon and redraws the rail. Empty `key` clears it.
  void SetPageIcon(int page_id, std::string_view key);
  // Runs a registered command through the shell's command resolution, doing
  // nothing when it does not resolve or is disabled. The rail's pages, its
  // pinned utilities and the page context menu all reach their commands this
  // way, so none of them can drift from what the menus and the Ctrl-K palette
  // do.
  void ExecuteShellCommand(unsigned command_id);

 public:
  // MainWindowInterface — raises the Settings overlay over this window,
  // creating it on first use. See `SettingsPanel`.
  //
  // Public for the screenshot generator, which opens the surface the way the
  // menu item and the rail utility do rather than constructing a panel of its
  // own — a capture that assembled its own form would document a surface the
  // client does not ship.
  void ShowSettings() override;

 private:
  // The height the Settings overlay must leave for the status strip, or 0 when
  // the strip is switched off. Re-measured rather than cached: `Status Bar` is
  // a row on that very surface.
  int StatusStripInset() const;

 public:
  // The shell's menu model. Exposed for the screenshot generator for the same
  // reason.
  scada::aui::MenuModel* main_menu_model() SCADA_LIFETIME_BOUND {
    return main_menu_model_.get();
  }

 private:
  // Re-derives the pinned-utility marker from the active view. A utility opens
  // a view in the current page rather than owning shell state, so the marker
  // is a projection of what the workspace is showing, like the mode marker.
  void RefreshUtilityMarker();
  // Opens an address-space tag (from the palette) in a table view.
  void OpenTag(const scada::NodeId& node_id, const std::u16string& title);
  // Starts the command palette's address-space browse. See the definition for
  // why it must not run before the first page is open.
  void StartTagSearchBrowse();
  // Opens the Ctrl-K command palette over every registered command, optionally
  // seeded with `initial_text` (type-to-search from the context-bar field).
  void ShowCommandPalette(const QString& initial_text = QString());
  void RebuildMenuBar();

  std::unique_ptr<ViewManager> view_manager_;

  // The QAction per button-surface command, kept in step with the handlers the
  // window resolves. Owned apart from the toolbar that displays them.
  std::unique_ptr<CommandActions> command_actions_;

  QToolBar* toolbar_ = nullptr;

  struct CategoryData {
    QMenu* menu = nullptr;
    QAction* toolbar_action = nullptr;
  };

  std::map<CommandCategory, CategoryData> category_actions_;

  DialogServiceImplQt dialog_service_;

  std::unique_ptr<scada::aui::MenuModel> main_menu_model_;

  std::unique_ptr<ProgressController> progress_controller_;

  // The Settings overlay, created on first use and kept afterwards so reopening
  // it is instant and so it keeps the operator's search text and scope tab.
  // Parented to this window and raised over everything below the status strip;
  // it is never in a layout, so it disturbs nothing it covers.
  SettingsPanel* settings_panel_ = nullptr;

  // Top context bar: the command/search field and alarm state. It
  // deliberately carries neither a brand mark nor identity/connection cells —
  // the window title names the application and the status strip owns who/where
  // (see CreateContextBar).
  QToolBar* context_bar_ = nullptr;
  CommandField* command_search_ = nullptr;
  // Where the workspace is (page → view → selection), in the bar's left slot.
  // Refreshed from the three places the path can move: UpdateTitle (page),
  // OnActiveViewChanged (view) and OnSelectionChanged (subject).
  Breadcrumb* breadcrumb_ = nullptr;
  // Drives the bar's alarm-state cluster (AlarmStateCluster) from the
  // status-bar model's counts.
  boost::signals2::scoped_connection context_bar_connection_;

  // Left activity rail. Selects the sidebar's pane mode.
  ActivityBar* activity_bar_ = nullptr;
  // The page list and switching policy behind the rail's pages group, shared
  // with the Page main menu so both obey the same rules.
  std::unique_ptr<PageSwitcher> page_switcher_;
  // The sidebar's mode policy, and the adapter it drives this window through.
  // Built with the rail, so both are null until CreateActivityBar has run.
  class PaneModeHostImpl;
  std::unique_ptr<PaneModeHostImpl> pane_mode_host_;
  std::unique_ptr<PaneModeController> pane_modes_;
  // Mirrors the last state asked for through SetWindowFlashing, so the alert is
  // raised on the rising edge only. OnEvents calls in on every event dispatch.
  bool window_flashing_ = false;
  // Empty in the shipping client; set only by the screenshot generator. See
  // SetPopupMenuInterceptor.
  PopupMenuInterceptor popup_menu_interceptor_;

  // Right Inspector dock. Updated from OnSelectionChanged with the
  // active view's SelectionModel.
  InspectorPanel* inspector_ = nullptr;
  // The Inspector's host dock, kept so the Device-diagnostics dock can tabify
  // onto it.
  QDockWidget* inspector_dock_ = nullptr;

  // The right dock's specialist panels, contributed by feature modules
  // through the SelectionPanelRegistry. Each is fed every selection change and
  // decides for itself which selections it shows.
  std::vector<std::unique_ptr<SelectionPanel>> selection_panels_;

  // Flat tag index for the command palette's tag search (null when no node
  // service is available).
  std::unique_ptr<TagSearchIndex> tag_search_index_;

  // Watches for a re-login, which resets the palette's tag index.
  boost::signals2::scoped_connection session_state_connection_;

  boost::signals2::scoped_connection change_profile_connection_;
};
