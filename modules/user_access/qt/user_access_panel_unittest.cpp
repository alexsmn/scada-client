#include "user_access/qt/user_access_panel.h"

#include "aui/test/app_environment.h"
#include "common/node_state.h"
#include "model/security_node_ids.h"
#include "node_service/test/fake_node_service.h"
#include "scada/node_id.h"
#include "scada/variant.h"

#include <gtest/gtest.h>

#include <QLabel>
#include <QStackedWidget>

#include <optional>

namespace {

class UserAccessPanelTest : public ::testing::Test {
 protected:
  AppEnvironment app_env_;
  FakeNodeService node_service_;

  // Registers UserType plus one instance of it. `access` mirrors what the
  // service has actually delivered for the AccessRights property:
  //   - a value          -> the resolved case;
  //   - an empty Variant -> the property node exists but nothing was ever read
  //                         for it (the state a selection handler sees when it
  //                         renders before the fetch lands);
  //   - nullopt          -> the property was not materialized at all, so the
  //                         aggregate lookup itself misses.
  NodeRef AddUser(std::optional<scada::Variant> access) {
    node_service_.Add(
        scada::NodeState{.node_id = scada::security::id::UserType,
                         .node_class = scada::NodeClass::ObjectType});

    scada::NodeState user{.node_id = scada::NodeId{7001, 1},
                          .node_class = scada::NodeClass::Object,
                          .type_definition_id = scada::security::id::UserType,
                          .attributes = {.display_name = u"Администратор"}};
    if (access) {
      // Seeded directly rather than through set_property(), which treats an
      // empty Variant as a removal — and an empty Variant is precisely the
      // undelivered case under test.
      user.properties.emplace_back(scada::security::id::UserType_AccessRights,
                                   std::move(*access));
    }
    return node_service_.Add(std::move(user));
  }

  static QString PillText(const UserAccessPanel& panel) {
    auto* pill = panel.findChild<QLabel*>(QStringLiteral("userRolePill"));
    return pill ? pill->text() : QString{};
  }

  // The permission rows carry no object name, so count them off the
  // placeholder's absence: ShowAccess draws exactly one placeholder when the
  // breakdown is empty.
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

// Regression: an AccessRights that was never delivered used to be folded into a
// zero bitmask by get_or<Int32>(0), and zero is a perfectly valid bitmask —
// Observer, view only. The panel therefore answered a question it had no data
// for, with a plausible and wrong answer, for a user who may well be an
// administrator. It must report the unresolved state instead.
TEST_F(UserAccessPanelTest, UndeliveredAccessRightsDoesNotReadAsObserver) {
  UserAccessPanel panel;
  panel.ShowUser(AddUser(scada::Variant{}));

  EXPECT_NE(PillText(panel), QStringLiteral("Observer"));
  EXPECT_EQ(PillText(panel), QStringLiteral("No data"));

  // Nor may it claim the permissions themselves: an unresolved bitmask says
  // nothing about View either way.
  EXPECT_TRUE(HasPermissionPlaceholder(panel));
  EXPECT_FALSE(HasLabel(panel, QStringLiteral("View & monitor")));

  // The identity is known independently of the rights, so it still shows.
  EXPECT_TRUE(HasLabel(panel, QString::fromUtf8("Администратор")));
}

// The same honesty requirement when the aggregate lookup misses outright — the
// user node is resident but its AccessRights property never materialized.
TEST_F(UserAccessPanelTest, UnresolvedAccessRightsAggregateReadsAsNoData) {
  UserAccessPanel panel;
  panel.ShowUser(AddUser(std::nullopt));

  EXPECT_EQ(PillText(panel), QStringLiteral("No data"));
  EXPECT_TRUE(HasPermissionPlaceholder(panel));
}

// The counterpart the fix must not disturb: a delivered bitmask still resolves
// to the real role and the full breakdown. AccessRights = 3 is Configure +
// Control, the users-rbac.png fixture's administrator.
TEST_F(UserAccessPanelTest, DeliveredAccessRightsResolvesTheRealRole) {
  UserAccessPanel panel;
  panel.ShowUser(AddUser(scada::Variant{static_cast<scada::Int32>(3)}));

  EXPECT_EQ(PillText(panel), QStringLiteral("Administrator"));
  EXPECT_FALSE(HasPermissionPlaceholder(panel));
  EXPECT_TRUE(HasLabel(panel, QStringLiteral("View & monitor")));
  EXPECT_TRUE(HasLabel(panel, QStringLiteral("Control & manual input")));
  EXPECT_TRUE(HasLabel(panel, QStringLiteral("Configure & administer")));
}

// A zero bitmask is a real answer, not an absent one — it must keep reading as
// Observer, which is what makes the undelivered case above worth telling apart.
TEST_F(UserAccessPanelTest, ZeroAccessRightsStillReadsAsObserver) {
  UserAccessPanel panel;
  panel.ShowUser(AddUser(scada::Variant{static_cast<scada::Int32>(0)}));

  EXPECT_EQ(PillText(panel), QStringLiteral("Observer"));
  EXPECT_FALSE(HasPermissionPlaceholder(panel));
  EXPECT_TRUE(HasLabel(panel, QStringLiteral("View & monitor")));
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
