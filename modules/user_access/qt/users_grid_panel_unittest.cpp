#include "user_access/qt/users_grid_panel.h"

#include "aui/test/app_environment.h"
#include "scada/node_id.h"

#include <gtest/gtest.h>

#include <QLabel>
#include <QTableWidget>

#include <optional>

namespace {

class UsersGridPanelTest : public ::testing::Test {
 protected:
  AppEnvironment app_env_;

  static std::vector<UserGridRow> SampleRows() {
    return {
        {scada::NodeId{234, 1}, u"root", UserRole::kObserver, false},
        {scada::NodeId{2, 1}, u"engineer", UserRole::kAdministrator, false},
        {scada::NodeId{3, 1}, u"dispatcher", UserRole::kOperator, true},
    };
  }
};

TEST_F(UsersGridPanelTest, ShowRowsPopulatesGridAndCount) {
  UsersGridPanel panel;
  panel.ShowRows(SampleRows());

  auto* grid = panel.findChild<QTableWidget*>(QStringLiteral("usersGrid"));
  ASSERT_NE(grid, nullptr);
  EXPECT_EQ(grid->rowCount(), 3);
  EXPECT_EQ(grid->item(1, 0)->text(), QStringLiteral("engineer"));
  EXPECT_EQ(grid->item(1, 1)->text(), QStringLiteral("Administrator"));
  EXPECT_EQ(grid->item(2, 2)->text(), QStringLiteral("Multiple"));
  EXPECT_EQ(grid->item(0, 2)->text(), QStringLiteral("Single"));

  auto* title = panel.findChild<QLabel*>(QStringLiteral("usersTitle"));
  ASSERT_NE(title, nullptr);
  EXPECT_TRUE(title->text().contains(QStringLiteral("3")));
}

TEST_F(UsersGridPanelTest, SelectingARowEmitsUserActivated) {
  UsersGridPanel panel;
  panel.ShowRows(SampleRows());

  std::optional<scada::NodeId> activated;
  QObject::connect(&panel, &UsersGridPanel::UserActivated,
                   [&](const scada::NodeId& id) { activated = id; });

  auto* grid = panel.findChild<QTableWidget*>(QStringLiteral("usersGrid"));
  ASSERT_NE(grid, nullptr);
  grid->selectRow(1);

  ASSERT_TRUE(activated.has_value());
  EXPECT_EQ(*activated, (scada::NodeId{2, 1}));
}

TEST_F(UsersGridPanelTest, EmptyRowsClearsGrid) {
  UsersGridPanel panel;
  panel.ShowRows(SampleRows());
  panel.ShowRows({});

  auto* grid = panel.findChild<QTableWidget*>(QStringLiteral("usersGrid"));
  ASSERT_NE(grid, nullptr);
  EXPECT_EQ(grid->rowCount(), 0);
}

}  // namespace
