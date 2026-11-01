#include "accounts/qt/account_roles_dialog_qt.h"

#include "aui/test/app_environment.h"
#include "scada/authorization.h"

#include <QListWidget>

#include <gtest/gtest.h>

namespace {

const scada::NodeId kObserver =
    scada::WellKnownRoleId(scada::WellKnownRole::kObserver);
const scada::NodeId kOperator =
    scada::WellKnownRoleId(scada::WellKnownRole::kOperator);
const scada::NodeId kConfigureAdmin =
    scada::WellKnownRoleId(scada::WellKnownRole::kConfigureAdmin);

class AccountRolesDialogTest : public ::testing::Test {
 protected:
  AppEnvironment app_env_;
};

// The dialog draws one checkbox per editable Role, in RoleSet order, checked
// exactly where the account holds the Role — a checkbox is a claim about
// membership.
TEST_F(AccountRolesDialogTest, ChecksTheRolesTheAccountHolds) {
  AccountRolesDialog dialog{
      u"ivanov",
      {{.role_id = kObserver, .name = u"Observer", .held = false},
       {.role_id = kOperator, .name = u"Operator", .held = true},
       {.role_id = kConfigureAdmin, .name = u"ConfigureAdmin", .held = false}}};

  ASSERT_EQ(dialog.list()->count(), 3);
  EXPECT_EQ(dialog.list()->item(1)->text(), QStringLiteral("Operator"));
  EXPECT_EQ(dialog.list()->item(0)->checkState(), Qt::Unchecked);
  EXPECT_EQ(dialog.list()->item(1)->checkState(), Qt::Checked);
  EXPECT_EQ(dialog.CheckedRoles(), std::vector<scada::NodeId>{kOperator});
}

// What the administrator leaves checked is what the command applies.
TEST_F(AccountRolesDialogTest, ReportsTheEditedSelection) {
  AccountRolesDialog dialog{
      u"ivanov",
      {{.role_id = kOperator, .name = u"Operator", .held = true},
       {.role_id = kConfigureAdmin, .name = u"ConfigureAdmin", .held = false}}};

  dialog.list()->item(0)->setCheckState(Qt::Unchecked);
  dialog.list()->item(1)->setCheckState(Qt::Checked);

  EXPECT_EQ(dialog.CheckedRoles(), std::vector<scada::NodeId>{kConfigureAdmin});
}

}  // namespace
