#pragma once

#include "node_service/node_ref.h"
#include "user_access/user_access.h"

#include <QString>
#include <QWidget>

#include <vector>

class QLabel;
class QStackedWidget;
class QVBoxLayout;

// One rendered permission row: its label and whether the user is granted it.
struct UserPermissionDisplay {
  QString label;
  bool granted;
};

// The reshell users-admin RBAC inspector — the right region of
// docs/product/ui-mockups/screens/users-admin.html. For a selected user it shows
// the identity, a coarse role pill (Administrator / Operator / Observer) and the
// permission breakdown (View / Control / Configure — granted or not), derived
// from the user's AccessRights bitmask.
//
// Selection flows in through ShowUser(): the host routes a UserType-node
// selection here.
//
// Opt-in: construct this only under the reshell UX theme (see
// MakeUserAccessPanel).
class UserAccessPanel : public QWidget {
  Q_OBJECT

 public:
  explicit UserAccessPanel(QWidget* parent = nullptr);
  ~UserAccessPanel() override;

  // Reflects `user` (expected to be a UserType instance): reads its AccessRights
  // and fills the role + permissions. A null / non-user node clears.
  void ShowUser(const NodeRef& user);

  // Clears to the empty state.
  void Clear();

  // Render primitive that ShowUser drives — the seam the widget tests exercise
  // without a node service.
  void ShowAccess(const QString& name,
                  UserRole role,
                  const std::vector<UserPermissionDisplay>& permissions);

 private:
  QWidget* BuildEmptyState();
  QWidget* BuildContent();

  QStackedWidget* stack_ = nullptr;  // [0] empty state, [1] content.
  QLabel* name_ = nullptr;
  QLabel* role_ = nullptr;
  QVBoxLayout* perms_layout_ = nullptr;  // owns the current permission rows.
};

// Builds a UserAccessPanel under the reshell UX theme
// (scada::aui::GetSeverityTheme() != SeverityTheme::kLegacy); returns nullptr in
// the legacy look. Ownership transfers to the caller.
UserAccessPanel* MakeUserAccessPanel();
