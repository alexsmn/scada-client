#include "user_access/qt/users_grid_panel.h"

#include "aui/test/app_environment.h"
#include "scada/node_id.h"

#include <gtest/gtest.h>

#include <QLabel>
#include <QPushButton>
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

TEST_F(UsersGridPanelTest, ResetPasswordFollowsSelectionAndEmitsActionsMenu) {
  UsersGridPanel panel;
  panel.ShowRows(SampleRows());

  const QList<QPushButton*> buttons = panel.findChildren<QPushButton*>();
  QPushButton* reset = nullptr;
  for (QPushButton* button : buttons) {
    if (button->text() == QStringLiteral("Reset password"))
      reset = button;
  }
  ASSERT_NE(reset, nullptr);
  // Disabled until a user is selected.
  EXPECT_FALSE(reset->isEnabled());

  auto* grid = panel.findChild<QTableWidget*>(QStringLiteral("usersGrid"));
  ASSERT_NE(grid, nullptr);
  grid->selectRow(1);
  EXPECT_TRUE(reset->isEnabled());

  bool right_click = true;
  int emitted = 0;
  QObject::connect(&panel, &UsersGridPanel::ActionsMenuRequested,
                   [&](const QPoint&, bool rc) {
                     ++emitted;
                     right_click = rc;
                   });
  reset->click();
  EXPECT_EQ(emitted, 1);
  EXPECT_FALSE(right_click);  // the button path, not a right-click.
}

TEST_F(UsersGridPanelTest, AddUserIsEnabledAndEmitsActionsMenu) {
  UsersGridPanel panel;
  panel.ShowRows(SampleRows());

  const QList<QPushButton*> buttons = panel.findChildren<QPushButton*>();
  QPushButton* add = nullptr;
  for (QPushButton* button : buttons) {
    if (button->text() == QStringLiteral("Add user"))
      add = button;
  }
  ASSERT_NE(add, nullptr);
  // Add-user is parent-scoped, so it is always enabled (access-right-gated at
  // the command level) — no user selection required.
  EXPECT_TRUE(add->isEnabled());

  int emitted = 0;
  QObject::connect(&panel, &UsersGridPanel::ActionsMenuRequested,
                   [&](const QPoint&, bool) { ++emitted; });
  add->click();
  EXPECT_EQ(emitted, 1);
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
