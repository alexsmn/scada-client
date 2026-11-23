#include "user_access/qt/roles_grid_panel.h"

#include "aui/qt/theme_qt.h"
#include "aui/test/app_environment.h"
#include "scada/authorization.h"

#include <QHeaderView>
#include <QLabel>
#include <QTableWidget>

#include <gtest/gtest.h>

namespace {

class RolesGridPanelTest : public ::testing::Test {
 protected:
  AppEnvironment app_env_;

  static std::vector<RoleMembership> SampleRoles() {
    return {
        {.name = u"Operator",
         .members = {u"ivanov", u"petrov"},
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

// A well-known Role reads as its translated label rather than the standard
// BrowseName; a custom Role keeps its own name (backlog 1129).
TEST_F(RolesGridPanelTest, WellKnownRolesShowTheirDisplayName) {
  RolesGridPanel panel;
  panel.ShowRoles(std::vector<RoleMembership>{
      {.node_id = scada::WellKnownRoleId(scada::WellKnownRole::kConfigureAdmin),
       .name = u"ConfigureAdmin",
       .well_known = true},
      {.node_id = scada::NodeId{7, 1}, .name = u"Shift A"}});

  auto* grid = panel.findChild<QTableWidget*>(QStringLiteral("rolesGrid"));
  ASSERT_NE(grid, nullptr);
  EXPECT_EQ(grid->item(0, 0)->text(),
            QStringLiteral("Configuration administrator"));
  EXPECT_EQ(grid->item(1, 0)->text(), QStringLiteral("Shift A"));
  EXPECT_EQ(grid->horizontalHeader()->sectionResizeMode(0),
            QHeaderView::ResizeToContents);
}

// The admin requirement is stated only to a session that lacks it; telling an
// administrator they need the Administrator role is wrong (backlog 1129).
TEST_F(RolesGridPanelTest, StatesTheAdminRequirementOnlyToNonAdministrators) {
  RolesGridPanel panel;

  auto* hint = panel.findChild<QLabel*>(QStringLiteral("rolesHint"));
  ASSERT_NE(hint, nullptr);
  EXPECT_FALSE(hint->text().isEmpty());
  EXPECT_FALSE(hint->isHidden());

  panel.SetCanAdminister(true);
  EXPECT_TRUE(hint->isHidden());

  panel.SetCanAdminister(false);
  EXPECT_FALSE(hint->isHidden());
}

}  // namespace
