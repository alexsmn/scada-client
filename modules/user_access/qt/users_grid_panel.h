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
// enabled) in a themed table, with a header carrying the count. Selecting a
// row emits UserActivated so the host can drive the RBAC inspector.
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
// Roles... / Delete, admin-gated) and the CATEGORY_CREATE "New" submenu whose
// "User" entry runs the existing create command (OpenedViewCreateCommand ->
// PostInsertTask, parented to the Users folder, gated on the Configure access
// right). Both buttons follow the session's Configure right (SetCanAdminister),
// which is what the Server checks before it lets an account administer others
// (backlog 1093): a session without it gets them disabled, with the reason
// beside them and on their tooltips, rather than a form the Server would
// refuse (docs/client/ux/principles.md §7). Reset-password further needs a
// selected user row. Enable / disable of an account is shown in the Status
// column (the Part 18 UserConfigurationMask) and changed by the Enable Account
// / Disable Account selection commands in the same menu (modules/accounts,
// backlog 149). The host re-reads the rows when an account, Role or role rule
// changes (UsersGridReloader), so the grid follows those commands.
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

  // Says the account list could not be read, instead of rendering an empty
  // grid that would state the server has no users. A non-administrator gets
  // this by design (OPC UA Part 18 §5.2.1), and so does everyone connected to
  // a Server older than 2.6, which publishes no UserManagement object.
  void ShowUnavailable();

  // Whether the panel currently shows the unavailable state.
  bool unavailable() const;

  // Whether this session may administer accounts — the Configure access right,
  // which every Role the Server accepts for user administration implies
  // (scada::AccessRightsForRoles). Without it Add user and Reset password are
  // disabled and the reason is shown; with it the reason is hidden, since it
  // would tell an administrator they are not one. Starts false: nothing is
  // offered until the host has said it may be.
  void SetCanAdminister(bool can_administer);

  bool can_administer() const { return can_administer_; }

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
  void UpdateActions();

  QLabel* title_ = nullptr;
  QLabel* unavailable_ = nullptr;
  QLabel* hint_ = nullptr;
  QPushButton* add_user_ = nullptr;
  QPushButton* reset_password_ = nullptr;
  QTableWidget* grid_ = nullptr;

  std::vector<UserGridRow> rows_;
  bool can_administer_ = false;
};

// Builds a UsersGridPanel. Ownership transfers to the caller.
UsersGridPanel* MakeUsersGridPanel();
