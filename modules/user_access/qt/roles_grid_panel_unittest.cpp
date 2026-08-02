#include "user_access/qt/roles_grid_panel.h"

#include "aui/qt/theme_qt.h"
#include "aui/test/app_environment.h"

#include <QLabel>
#include <QTableWidget>

#include <gtest/gtest.h>

namespace {

class RolesGridPanelTest : public ::testing::Test {
 protected:
  AppEnvironment app_env_;

  static std::vector<RoleMembership> SampleRoles() {
    return {
        {.name = u"Operator", .members = {u"ivanov", u"petrov"},
         .well_known = true},
        // A Role nobody holds, and a custom one.
        {.name = u"Engineer", .members = {}, .well_known = true},
        {.name = u"Shift A", .members = {u"kozlov"}, .well_known = false},
    };
  }
};

TEST_F(RolesGridPanelTest, ShowRolesPopulatesGridAndCount) {
  RolesGridPanel panel;
  panel.ShowRoles(SampleRoles());

  auto* grid = panel.findChild<QTableWidget*>(QStringLiteral("rolesGrid"));
  ASSERT_NE(grid, nullptr);
  EXPECT_EQ(grid->rowCount(), 3);
  EXPECT_EQ(grid->item(0, 0)->text(), QStringLiteral("Operator"));
  EXPECT_EQ(grid->item(0, 1)->text(), QStringLiteral("ivanov, petrov"));
  EXPECT_EQ(grid->item(0, 2)->text(), QStringLiteral("Standard role"));
  // A Role nobody holds is a real state, shown as such.
  EXPECT_EQ(grid->item(1, 1)->text(), QStringLiteral("None"));
  EXPECT_EQ(grid->item(2, 2)->text(), QStringLiteral("Custom role"));

  auto* title = panel.findChild<QLabel*>(QStringLiteral("rolesTitle"));
  ASSERT_NE(title, nullptr);
  EXPECT_TRUE(title->text().contains(QStringLiteral("3")));
}

// A RoleSet that could not be read must not render as a server with no Roles:
// a conformant server always publishes the well-known ones.
TEST_F(RolesGridPanelTest, UnreadableRoleSetSaysSoRatherThanShowingNone) {
  RolesGridPanel panel;
  panel.ShowRoles(std::nullopt);

  auto* grid = panel.findChild<QTableWidget*>(QStringLiteral("rolesGrid"));
  ASSERT_NE(grid, nullptr);
  EXPECT_EQ(grid->rowCount(), 0);
  EXPECT_TRUE(panel.roles().empty());

  auto* title = panel.findChild<QLabel*>(QStringLiteral("rolesTitle"));
  ASSERT_NE(title, nullptr);
  EXPECT_TRUE(title->text().contains(QStringLiteral("No data")));
  EXPECT_FALSE(title->text().contains(QStringLiteral("0")));
}

// The admin gate is stated, matching WIN_REQUIRES_ADMIN on the view.
TEST_F(RolesGridPanelTest, StatesTheAdminRequirement) {
  RolesGridPanel panel;

  auto* hint = panel.findChild<QLabel*>(QStringLiteral("rolesHint"));
  ASSERT_NE(hint, nullptr);
  EXPECT_FALSE(hint->text().isEmpty());
}

}  // namespace
