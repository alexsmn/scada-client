#pragma once

#include "base/any_executor.h"
#include "node_service/node_ref.h"
#include "user_access/user_access.h"

#include <QString>
#include <QStringList>
#include <QWidget>

#include <memory>
#include <optional>
#include <vector>

class QLabel;
class QStackedWidget;
class QVBoxLayout;

// One rendered permission row: its label and whether the user is granted it.
struct UserPermissionDisplay {
  QString label;
  bool granted;
};

// The users-admin RBAC inspector — the right region of
// docs/product/ui-mockups/screens/users-admin.html. For a selected account it
// shows the identity, the Roles it holds, and the permission breakdown
// (View / Control / Configure) those Roles imply.
//
// It shows ROLES, not a derived tier. Authorization is role-based (OPC UA
// Part 18 §4.4.1) and the old two-bit AccessRights mask is vestigial, so a
// pill reading "Administrator" off that mask would keep asserting authority
// for an account whose Role had been revoked. The permissions come from the
// same default role→permission map the server enforces with, so the panel and
// the server cannot disagree.
//
// Selection flows in through ShowUser(): the shell routes a UserType-node
// selection here. The node is used ONLY for the account's name — the standard
// user model keys by name, and a Role membership rule names an account, not a
// node. Everything shown is then read from the RoleSet.
//
// Opt-in: construct this only under the reshell UX theme (see
// MakeUserAccessPanel).
class UserAccessPanel : public QWidget {
  Q_OBJECT

 public:
  explicit UserAccessPanel(QWidget* parent = nullptr);
  ~UserAccessPanel() override;

  // Reflects `user` (expected to be a UserType instance), reading its Roles
  // from the RoleSet through `node_service`. A null / non-user node clears.
  //
  // The read is asynchronous, so the panel shows the account immediately with
  // its Roles pending, then fills them. Until they arrive the Roles read "No
  // data" and NO permission row is drawn — an unresolved role set says nothing
  // about the individual permissions, so drawing them ungranted would be as
  // false a claim as drawing them granted (docs/client/ux/principles.md §5).
  void ShowUser(const NodeRef& user,
                NodeService& node_service,
                AnyExecutor executor);

  // Reflects an account by name, with its Roles already resolved. `roles` is
  // nullopt when the RoleSet could not be read.
  void ShowAccount(const QString& name,
                   const std::optional<std::vector<AccountRole>>& roles);

  // Clears to the empty state.
  void Clear();

  // Render primitive that ShowAccount drives — the seam the widget tests
  // exercise without a node service. An empty `roles` list renders the
  // explicit "no roles" state; an empty `permissions` list renders the
  // "cannot be determined" placeholder.
  void ShowAccess(const QString& name,
                  const QStringList& roles,
                  const std::vector<UserPermissionDisplay>& permissions);

 private:
  QWidget* BuildEmptyState();
  QWidget* BuildContent();

  // Guards a late role read against a destroyed panel and against a newer
  // selection landing first.
  std::shared_ptr<int> lifetime_token_ = std::make_shared<int>(0);
  QString pending_name_;

  QStackedWidget* stack_ = nullptr;  // [0] empty state, [1] content.
  QLabel* name_ = nullptr;
  QLabel* roles_ = nullptr;
  QVBoxLayout* perms_layout_ = nullptr;  // owns the current permission rows.
};

// Builds a UserAccessPanel under the reshell UX theme
// (scada::aui::GetSeverityTheme() != SeverityTheme::kLegacy); returns nullptr in
// the legacy look. Ownership transfers to the caller.
UserAccessPanel* MakeUserAccessPanel();
