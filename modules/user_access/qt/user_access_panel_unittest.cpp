#include "user_access/qt/user_access_panel.h"

#include "aui/test/app_environment.h"

#include <gtest/gtest.h>

#include <QLabel>
#include <QStackedWidget>

namespace {

class UserAccessPanelTest : public ::testing::Test {
 protected:
  AppEnvironment app_env_;
};

TEST_F(UserAccessPanelTest, StartsInEmptyState) {
  UserAccessPanel panel;
  auto* stack = panel.findChild<QStackedWidget*>();
  ASSERT_NE(stack, nullptr);
  EXPECT_EQ(stack->currentIndex(), 0);
}

TEST_F(UserAccessPanelTest, ShowAccessFillsRolePillAndPermissions) {
  UserAccessPanel panel;
  panel.ShowAccess(QStringLiteral("root"), UserRole::kAdministrator,
                   {{QStringLiteral("View"), true},
                    {QStringLiteral("Control"), true},
                    {QStringLiteral("Configure"), true}});

  auto* stack = panel.findChild<QStackedWidget*>();
  ASSERT_NE(stack, nullptr);
  EXPECT_EQ(stack->currentIndex(), 1);

  auto* pill = panel.findChild<QLabel*>(QStringLiteral("userRolePill"));
  ASSERT_NE(pill, nullptr);
  EXPECT_EQ(pill->text(), QStringLiteral("Administrator"));

  const QList<QLabel*> labels = panel.findChildren<QLabel*>();
  bool has_name = false;
  bool has_configure = false;
  for (const QLabel* label : labels) {
    if (label->text() == QStringLiteral("root"))
      has_name = true;
    if (label->text() == QStringLiteral("Configure"))
      has_configure = true;
  }
  EXPECT_TRUE(has_name);
  EXPECT_TRUE(has_configure);
}

TEST_F(UserAccessPanelTest, ObserverRolePillReads) {
  UserAccessPanel panel;
  panel.ShowAccess(QStringLiteral("audit"), UserRole::kObserver,
                   {{QStringLiteral("View"), true}});
  auto* pill = panel.findChild<QLabel*>(QStringLiteral("userRolePill"));
  ASSERT_NE(pill, nullptr);
  EXPECT_EQ(pill->text(), QStringLiteral("Observer"));
}

TEST_F(UserAccessPanelTest, ClearReturnsToEmptyState) {
  UserAccessPanel panel;
  panel.ShowAccess(QStringLiteral("u"), UserRole::kObserver, {});
  panel.Clear();
  auto* stack = panel.findChild<QStackedWidget*>();
  ASSERT_NE(stack, nullptr);
  EXPECT_EQ(stack->currentIndex(), 0);
}

}  // namespace
