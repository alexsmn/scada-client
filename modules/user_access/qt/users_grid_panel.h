#pragma once

#include "base/lifetime.h"
#include "scada/node_id.h"
#include "user_access/users_grid.h"

#include <QPoint>
#include <QWidget>

#include <vector>

class QLabel;
class QPushButton;
class QTableWidget;

// The reshell users-admin grid — the main region of users-admin.html. It lists
// every account (name, description, the Roles it holds, and whether it is
// enabled) in a themed table, with a header carrying the count and the
// "editing requires Administrator" hint. Selecting a row emits UserActivated
// so the host can drive the RBAC inspector.
//
// The rows come from the OPC UA standard user model — one Read of
// UserManagement.Users joined against the RoleSet membership rules (see
// users_grid.h). The Roles column is the authorization model itself, not a
// label derived from the retired access-rights bitmask, so revoking a Role is
// visible here.
//
// Actions reuse the existing commands: a right-click on the grid, the
// Reset-password button, or the Add-user button emits ActionsMenuRequested so
// the host pops the standard view context menu through
// ControllerDelegate::ShowPopupMenu — no bespoke write path. That menu carries
// both the selection commands for the highlighted user (Set Password... /
// Delete, admin-gated) and the CATEGORY_CREATE "New" submenu whose "User" entry
// runs the existing create command (OpenedViewCreateCommand -> PostInsertTask,
// parented to the Users folder, gated on the Configure access right). Reset-
// password is enabled only when a user row is selected; Add-user is always
// enabled (the create is parent-scoped and access-right-gated at the command).
// Enable / disable of an account is SHOWN (the Status column, from the Part 18
// UserConfigurationMask) but not yet editable from here: the standard
// ModifyUser call that would toggle it is not wired to a command, so offering
// a toggle would promise a write the panel cannot make.
//
// It is returned as the Users view (see
// NodeTableController); UiView is a QWidget, so the panel is the view.
class UsersGridPanel : public QWidget {
  Q_OBJECT

 public:
  explicit UsersGridPanel(QWidget* parent = nullptr);
  ~UsersGridPanel() override;

  // Renders the given rows into the grid and updates the header count. The seam
  // the widget tests and the controller's browse both drive.
  void ShowRows(const std::vector<UserGridRow>& rows);

  const std::vector<UserGridRow>& rows() const SCADA_LIFETIME_BOUND {
    return rows_;
  }

 Q_SIGNALS:
  // Emitted when the operator activates (selects) a user row. Carries the
  // account's UserType node, which the shell routes to the RBAC inspector —
  // the node is a carrier for the account's NAME, not a source of rights.
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

// Builds a UsersGridPanel. Ownership transfers to the caller.
UsersGridPanel* MakeUsersGridPanel();
