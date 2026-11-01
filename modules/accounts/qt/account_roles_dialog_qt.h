#pragma once

#include "accounts/account_roles_dialog.h"

#include <QDialog>

#include <vector>

class QListWidget;

// The Roles dialog: one checkbox per editable Role, checked where the account
// holds it. Exposed for the widget tests and the screenshot generator; the
// command reaches it through ShowAccountRolesDialog.
class AccountRolesDialog : public QDialog {
  Q_OBJECT

 public:
  AccountRolesDialog(const std::u16string& account,
                     const std::vector<AccountRoleChoice>& choices,
                     QWidget* parent = nullptr);
  ~AccountRolesDialog() override;

  // The Roles currently checked, in row order.
  std::vector<scada::NodeId> CheckedRoles() const;

  QListWidget* list() const { return list_; }

 private:
  std::vector<AccountRoleChoice> choices_;
  QListWidget* list_ = nullptr;
};
