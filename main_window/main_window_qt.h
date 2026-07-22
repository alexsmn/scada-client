#pragma once

#include "aui/qt/dialog_service_impl_qt.h"
#include "controller/action_manager.h"
#include "main_window/base_main_window.h"

#include <QMainWindow>

namespace events {
class SeverityTileStrip;
}

#include <boost/signals2/connection.hpp>
#include <vector>

namespace scada {
class NodeId;
}

class ActivityBar;
class DeviceDiagnosticsPanel;
class InspectorPanel;
class UserAccessPanel;
class TransmissionRuleInspector;
class TagSearchIndex;
class QAction;
class QDockWidget;
class QLabel;
class QLineEdit;
class QMenu;
class QToolBar;
class QWidget;
class ProgressController;
class ViewManager;

class MainWindow final : public QMainWindow, public BaseMainWindow {
  Q_OBJECT

 public:
  explicit MainWindow(MainWindowContext&& context);
  ~MainWindow();

  // BaseMainWindow
  virtual DialogService& GetDialogService() override { return dialog_service_; }
  virtual void SetWindowFlashing(bool flashing) override;
  virtual void ShowPopupMenu(scada::aui::MenuModel* merge_menu,
                             unsigned resource_id,
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

  // QWidget
  virtual void closeEvent(QCloseEvent* event) override;

  // QObject
  virtual bool eventFilter(QObject* watched, QEvent* event) override;

 private:
  void CreateMenuBar();
  // Persists the experimental-reshell opt-in (Ux/Experimental) and tells the
  // operator a restart is needed, since theming installs at startup.
  void OnToggleExperimentalUx(bool enabled);
  void CreateToolbar();
  void CreateStatusBar();
  // Opt-in top context bar (brand + command/search + live context cluster).
  // Only built when the experimental UX is enabled; see main.cpp.
  void CreateContextBar();
  // Opt-in left activity rail (backlog 1.1): section navigation + alarm badge.
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
  // Opens the section's default view and marks it active on the rail.
  void ActivateSection(const std::string& window_info_name);
  // Opens the operator Overview page (from the rail's Overview section).
  void OpenOverviewPage();
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

  // Top context bar (opt-in). Its right-hand cluster mirrors the status-bar
  // model panes; `context_panes_` are the labels, refreshed on model changes.
  QToolBar* context_bar_ = nullptr;
  QLineEdit* command_search_ = nullptr;
  std::vector<QLabel*> context_panes_;
  // Status-bar pane index shown by each context_panes_ label (the curated
  // who/where subset), parallel to context_panes_.
  std::vector<int> context_pane_indices_;
  // Live severity KPI tiles in the context bar (critical / warning /
  // unacknowledged), refreshed with the status-bar model.
  events::SeverityTileStrip* severity_tiles_ = nullptr;
  // Alarm-flood escalation pill; visible only while a flood is active.
  QLabel* flood_indicator_ = nullptr;
  boost::signals2::scoped_connection context_bar_connection_;

  // Left activity rail (opt-in). Its alarm badge follows the status-bar model.
  ActivityBar* activity_bar_ = nullptr;
  boost::signals2::scoped_connection activity_bar_connection_;

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
