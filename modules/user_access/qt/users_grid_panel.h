#pragma once

#include "scada/node_id.h"
#include "user_access/users_grid.h"

#include <QPoint>
#include <QWidget>

#include <vector>

class QLabel;
class QPushButton;
class QTableWidget;

// The reshell users-admin grid — the main region of users-admin.html. It lists
// the users (identity + coarse role + session policy) in a themed table, with a
// header carrying the count and the "editing requires Administrator" hint.
// Selecting a row emits UserActivated so the host can drive the RBAC inspector.
//
// Actions reuse the existing selection commands: a right-click on a row, or the
// Reset-password button, emits ActionsMenuRequested so the host can pop the
// standard selection context menu (Set Password... / New / Delete) for the
// selected user through ControllerDelegate::ShowPopupMenu — no bespoke write
// path. Reset-password is enabled only when a user row is selected. Enable /
// disable of an account is *not* offered: the client UserType node model has no
// enabled/disabled attribute (it carries only AccessRights + MultiSessions +
// profile fields), so there is nothing to toggle; that state is server-side.
// Add-user (a parent-scoped create) is left as a disabled affordance.
//
// It is returned as the Users view under the reshell theme (see
// NodeTableController); UiView is a QWidget, so the panel is the view.
class UsersGridPanel : public QWidget {
  Q_OBJECT

 public:
  explicit UsersGridPanel(QWidget* parent = nullptr);
  ~UsersGridPanel() override;

  // Renders the given rows into the grid and updates the header count. The seam
  // the widget tests and the controller's browse both drive.
  void ShowRows(const std::vector<UserGridRow>& rows);

  const std::vector<UserGridRow>& rows() const { return rows_; }

 Q_SIGNALS:
  // Emitted when the operator activates (selects) a user row.
  void UserActivated(const scada::NodeId& user_id);

  // Emitted when the operator asks for the selected user's actions — a
  // right-click on a row (right_click = true) or the Reset-password button
  // (false). The host pops the standard selection context menu at global_pos.
  void ActionsMenuRequested(const QPoint& global_pos, bool right_click);

 private:
  QWidget* BuildHeader();
  QWidget* BuildGrid();
  void OnSelectionChanged();
  void OnContextMenuRequested(const QPoint& pos);
  bool HasSelection() const;

  QLabel* title_ = nullptr;
  QPushButton* add_user_ = nullptr;
  QPushButton* reset_password_ = nullptr;
  QTableWidget* grid_ = nullptr;

  std::vector<UserGridRow> rows_;
};

// Builds a UsersGridPanel under the reshell UX theme
// (scada::aui::GetSeverityTheme() != SeverityTheme::kLegacy); returns nullptr in
// the legacy look. Ownership transfers to the caller.
UsersGridPanel* MakeUsersGridPanel();
