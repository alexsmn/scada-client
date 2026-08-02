#pragma once

#include "user_access/role_membership.h"

#include <QWidget>

#include <vector>

class QLabel;
class QTableWidget;

// The Roles view: every Role of Server.ServerCapabilities.RoleSet and the
// accounts it is granted to — the "Roles" section of the Administration
// explorer (`docs/product/ui-mockups/screens/users-admin.html`).
//
// This is the authorization model made visible. Since the access-rights
// bitmask stopped being authoritative, a Role's membership is what actually
// decides what a session may do, and until now the client had no surface that
// showed it at all: an administrator could grant or revoke a Role and see
// nothing anywhere.
//
// Read-only. Editing membership is AddIdentity/RemoveIdentity on the Role
// (OPC UA Part 18 §4.4.5/§4.4.6), which is a privileged write this view does
// not yet route — showing an editable cell would promise a call it cannot
// make.
//
// UiView is a QWidget, so the panel is the view.
class RolesGridPanel : public QWidget {
  Q_OBJECT

 public:
  explicit RolesGridPanel(QWidget* parent = nullptr);
  ~RolesGridPanel() override;

  // Renders the Roles. `nullopt` means the RoleSet could not be read, which is
  // shown as such rather than as a server with no Roles — the distinction the
  // read path preserves (role_membership.h).
  void ShowRoles(const std::optional<std::vector<RoleMembership>>& roles);

  const std::vector<RoleMembership>& roles() const { return roles_; }

 private:
  QWidget* BuildHeader();
  QWidget* BuildGrid();

  QLabel* title_ = nullptr;
  QLabel* hint_ = nullptr;
  QTableWidget* grid_ = nullptr;

  std::vector<RoleMembership> roles_;
};

// Builds a RolesGridPanel under the reshell UX theme; returns nullptr in the
// legacy look. Ownership transfers to the caller.
RolesGridPanel* MakeRolesGridPanel();
