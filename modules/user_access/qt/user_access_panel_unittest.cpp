#include "user_access/qt/user_access_panel.h"

#include "aui/test/app_environment.h"
#include "scada/authorization.h"
#include "scada/node_id.h"

#include <gtest/gtest.h>

#include <QLabel>
#include <QStackedWidget>

#include <optional>
#include <vector>

namespace {

class UserAccessPanelTest : public ::testing::Test {
 protected:
  AppEnvironment app_env_;

  static AccountRole Role(scada::WellKnownRole role, std::u16string name) {
    return AccountRole{scada::WellKnownRoleId(role), std::move(name)};
  }

  static QString RolesText(const UserAccessPanel& panel) {
    auto* roles = panel.findChild<QLabel*>(QStringLiteral("userRoles"));
    return roles ? roles->text() : QString{};
  }

  // The permission rows carry no object name, so detect them off the
  // placeholder: ShowAccess draws exactly one when the breakdown is empty.
  static bool HasPermissionPlaceholder(const UserAccessPanel& panel) {
    return panel.findChild<QLabel*>(
               QStringLiteral("userPermissionsPlaceholder")) != nullptr;
  }

  static bool HasLabel(const UserAccessPanel& panel, const QString& text) {
    for (const QLabel* label : panel.findChildren<QLabel*>()) {
      if (label->text() == text)
        return true;
    }
    return false;
  }
};

TEST_F(UserAccessPanelTest, StartsInEmptyState) {
  UserAccessPanel panel;
  auto* stack = panel.findChild<QStackedWidget*>();
  ASSERT_NE(stack, nullptr);
  EXPECT_EQ(stack->currentIndex(), 0);
}

// The panel lists the Roles an account holds, not a derived tier: an account
// can hold several, and collapsing them into one is the fiction the retired
// access-rights bitmask told.
TEST_F(UserAccessPanelTest, ShowsEveryRoleHeld) {
  UserAccessPanel panel;
  panel.ShowAccount(
      QStringLiteral("root"),
      std::vector<AccountRole>{
          Role(scada::WellKnownRole::kOperator, u"Operator"),
          Role(scada::WellKnownRole::kConfigureAdmin, u"ConfigureAdmin")});

  auto* stack = panel.findChild<QStackedWidget*>();
  ASSERT_NE(stack, nullptr);
  EXPECT_EQ(stack->currentIndex(), 1);
  EXPECT_EQ(RolesText(panel), QStringLiteral("Operator, ConfigureAdmin"));
  EXPECT_TRUE(HasLabel(panel, QStringLiteral("root")));
}

// Permissions come from the Roles, through the same map the server enforces
// with — so an Operator shows Control but not Configure.
TEST_F(UserAccessPanelTest, DerivesPermissionsFromTheRolesHeld) {
  UserAccessPanel panel;
  panel.ShowAccount(QStringLiteral("ivanov"),
                    std::vector<AccountRole>{
                        Role(scada::WellKnownRole::kOperator, u"Operator")});

  EXPECT_FALSE(HasPermissionPlaceholder(panel));
  EXPECT_TRUE(HasLabel(panel, QStringLiteral("View & monitor")));
  EXPECT_TRUE(HasLabel(panel, QStringLiteral("Control & manual input")));
  EXPECT_TRUE(HasLabel(panel, QStringLiteral("Configure & administer")));
}

// Regression, carried over from the bitmask model and still the point: an
// unresolved role set must not be drawn as "holds nothing". It says nothing
// about the account, which may well be an administrator, and it says nothing
// about the individual permissions either — so no permission row is drawn at
// all (docs/client/ux/principles.md §5).
TEST_F(UserAccessPanelTest, UnreadableRoleSetDoesNotReadAsNoRoles) {
  UserAccessPanel panel;
  panel.ShowAccount(QStringLiteral("Администратор"), std::nullopt);

  EXPECT_NE(RolesText(panel), QStringLiteral("None"));
  EXPECT_EQ(RolesText(panel), QStringLiteral("No data"));
  EXPECT_TRUE(HasPermissionPlaceholder(panel));
  EXPECT_FALSE(HasLabel(panel, QStringLiteral("View & monitor")));

  // The identity is known independently of the Roles, so it still shows.
  EXPECT_TRUE(HasLabel(panel, QString::fromUtf8("Администратор")));
}

// The counterpart that makes the case above worth telling apart: an account
// that genuinely holds no Role is a real, ordinary state — and under the role
// model it grants nothing, which the breakdown states explicitly rather than
// omitting.
TEST_F(UserAccessPanelTest, NoRolesIsARealStateDistinctFromUnknown) {
  UserAccessPanel panel;
  panel.ShowAccount(QStringLiteral("audit"), std::vector<AccountRole>{});

  EXPECT_EQ(RolesText(panel), QStringLiteral("None"));
  // Permissions ARE known here — all denied — so the rows are drawn.
  EXPECT_FALSE(HasPermissionPlaceholder(panel));
  EXPECT_TRUE(HasLabel(panel, QStringLiteral("View & monitor")));
}

TEST_F(UserAccessPanelTest, ClearReturnsToEmptyState) {
  UserAccessPanel panel;
  panel.ShowAccount(QStringLiteral("u"), std::vector<AccountRole>{});
  panel.Clear();
  auto* stack = panel.findChild<QStackedWidget*>();
  ASSERT_NE(stack, nullptr);
  EXPECT_EQ(stack->currentIndex(), 0);
}

}  // namespace
