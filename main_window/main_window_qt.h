#pragma once

#include "aui/qt/dialog_service_impl_qt.h"
#include "controller/action_manager.h"
#include "main_window/base_main_window.h"
#include "main_window/pages/page_switcher.h"
#include "main_window/pane_modes.h"

#include <QMainWindow>

namespace events {
class SeverityTileStrip;
}

#include <boost/signals2/connection.hpp>
#include <string>
#include <string_view>
#include <vector>

namespace scada {
class NodeId;
}

class ActivityBar;
class Breadcrumb;
class CommandField;
class PageSwitcher;
class DeviceDiagnosticsPanel;
class InspectorPanel;
class UserAccessPanel;
class TransmissionRuleInspector;
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
  virtual void ShowPopupMenu(scada::aui::MenuModel* merge_menu,
                             const scada::aui::Point& point,
                             bool right_click) override;

 protected:
  // BaseMainWindow
  virtual void UpdateTitle() override;
  virtual void OnSelectionChanged() override;
  virtual void SetToolbarPosition(unsigned position) override;
  virtual std::unique_ptr<OpenedView> OnCreateView(
      WindowDefinition& def) override;

  // ViewManagerDelegate
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
  void CreateToolbar();
  void CreateStatusBar();
  // Opt-in top context bar: the command/search field and alarm state.
  // Only built when the experimental UX is enabled; see main.cpp.
  void CreateContextBar();
  // Re-derives the context bar's breadcrumb: page → active view → selected
  // object. Cheap and idempotent, so it is called from every hook that can move
  // any of the three rather than trying to work out which one moved.
  void RefreshBreadcrumb();
  // Opt-in left activity rail (backlog 1.1): selects which panes occupy the
  // left sidebar. It never opens a workspace tab and never switches the page.
  void CreateActivityBar();
  // Opt-in right Inspector dock (backlog 2.6): reflects the active view's
  // selection — identity, live value, control action.
  // Why the selection-scoped control command is unavailable, for the
  // Inspector's disabled Control button. Empty when nothing is selected.
  QString ControlUnavailableReason();
  void CreateInspectorPanel();
  // Opt-in right Device-diagnostics dock (backlog 5.0): reflects a selected
  // device's link status + live traffic/polling counters. Tabified with the
  // Inspector dock.
  void CreateDiagnosticsPanel();
  // Opt-in right RBAC dock: reflects a selected user's role + permissions.
  void CreateUserAccessPanel();
  // Opt-in right Transmission-rule dock: reflects a selected transmission item
  // (source → destination IOA). Tabified with the Inspector dock.
  void CreateTransmissionRulePanel();
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
  // Brings the current page's panes into line with the active mode, without
  // touching the persisted choice. Called on every page open.
  void ApplyPaneModeToCurrentWindow();
  // The mode the window is currently in, resolved from the profile preference
  // and falling back to what the open page looks like.
  PaneModeId ActivePaneMode();
  // Whether the current user may open `id`. Admin-gated modes resolve through
  // the same command router the menus use, so both agree by construction.
  bool IsPaneModeAvailable(PaneModeId id);
  // Re-derives the rail's active marker and per-mode availability from the
  // panes that are actually open, so the marker cannot go stale.
  void RefreshPaneModeMarker();
  // Raises the mode's own subject — the first pane it declares — above its
  // tabified siblings. Needed on a mode switch, where Qt would otherwise leave
  // whichever dock it tabified last on top, and on a page open, where the
  // restored dock blob carries the previously-fronted tab.
  void FrontPrimaryPane(const PaneMode& mode);
  // Opens an address-space tag (from the palette) in a table view.
  void OpenTag(const scada::NodeId& node_id, const std::u16string& title);
  // Opens the Ctrl-K command palette over every registered command, optionally
  // seeded with `initial_text` (type-to-search from the context-bar field).
  void ShowCommandPalette(const QString& initial_text = QString());
  void RebuildMenuBar();

  QAction* FindAction(unsigned command_id);

  void UpdateAction(QAction& qaction,
                    unsigned command_id,
                    ActionChangeMask change_mask);
  void UpdateMenuActions(QMenu& menu);

  void OnActionChanged(Action& action, ActionChangeMask change_mask);

  std::unique_ptr<ViewManager> view_manager_;

  std::map<unsigned /*command_id*/, QAction*> action_map_;
  std::map<QAction*, unsigned /*command_id*/> action_command_ids_;

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

  // Top context bar (opt-in): the command/search field and alarm state. It
  // deliberately carries neither a brand mark nor identity/connection cells —
  // the window title names the application and the status strip owns who/where
  // (see CreateContextBar).
  QToolBar* context_bar_ = nullptr;
  CommandField* command_search_ = nullptr;
  // Where the workspace is (page → view → selection), in the bar's left slot.
  // Refreshed from the three places the path can move: UpdateTitle (page),
  // OnActiveViewChanged (view) and OnSelectionChanged (subject).
  Breadcrumb* breadcrumb_ = nullptr;
  // Live severity KPI tiles in the context bar (critical / warning /
  // unacknowledged), refreshed with the status-bar model.
  events::SeverityTileStrip* severity_tiles_ = nullptr;
  // Alarm-flood escalation pill; visible only while a flood is active.
  QLabel* flood_indicator_ = nullptr;
  boost::signals2::scoped_connection context_bar_connection_;

  // Left activity rail (opt-in). Selects the sidebar's pane mode.
  ActivityBar* activity_bar_ = nullptr;
  // The page list and switching policy behind the rail's pages group, shared
  // with the Page main menu so both obey the same rules.
  std::unique_ptr<PageSwitcher> page_switcher_;
  // Guards RefreshPaneModeMarker against the pane close/activate notifications
  // that SetPaneMode itself provokes while it is mid-switch.
  bool applying_pane_mode_ = false;

  // Right Inspector dock (opt-in). Updated from OnSelectionChanged with the
  // active view's SelectionModel.
  InspectorPanel* inspector_ = nullptr;
  // The Inspector's host dock, kept so the Device-diagnostics dock can tabify
  // onto it.
  QDockWidget* inspector_dock_ = nullptr;

  // Right Device-diagnostics dock (opt-in). Filled from OnSelectionChanged when
  // the active view's selection is a device; cleared otherwise.
  DeviceDiagnosticsPanel* diagnostics_ = nullptr;

  // Right RBAC dock (opt-in). Filled from OnSelectionChanged when a user node
  // is selected.
  UserAccessPanel* user_access_ = nullptr;

  // Right Transmission-rule dock (opt-in). Filled from OnSelectionChanged when
  // the selection is a transmission item; cleared otherwise.
  TransmissionRuleInspector* transmission_rule_ = nullptr;

  // Flat tag index for the command palette's tag search (opt-in; null when the
  // reshell is off or no node service is available).
  std::unique_ptr<TagSearchIndex> tag_search_index_;

  boost::signals2::scoped_connection change_profile_connection_;
  boost::signals2::scoped_connection action_changed_connection_;
};
