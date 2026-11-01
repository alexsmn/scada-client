#include "account_roles/account_roles_dialog.h"

#include "scada/authorization.h"

#include <gtest/gtest.h>

namespace {

// The dialog offers every Role whose membership the server lets an
// administrator edit, marks the ones the account holds, and leaves out the two
// token-class Roles the server would refuse to change (Part 18 §4.3).
TEST(AccountRoleChoicesTest, OffersEditableRolesMarkedByMembership) {
  const scada::NodeId authenticated =
      scada::WellKnownRoleId(scada::WellKnownRole::kAuthenticatedUser);
  const scada::NodeId op =
      scada::WellKnownRoleId(scada::WellKnownRole::kOperator);
  const scada::NodeId admin =
      scada::WellKnownRoleId(scada::WellKnownRole::kConfigureAdmin);
  const scada::NodeId shift{7, 30};
  const std::vector<RoleMembership> roles = {
      {.node_id = authenticated, .name = u"AuthenticatedUser"},
      {.node_id = op, .name = u"Operator", .members = {u"petrov"}},
      {.node_id = admin,
       .name = u"ConfigureAdmin",
       .members = {u"petrov", u"ivanov"}},
      {.node_id = shift, .name = u"Shift A", .members = {u"ivanov"}}};

  const auto choices = MakeAccountRoleChoices(roles, u"ivanov");

  ASSERT_EQ(choices.size(), 3u);
  EXPECT_EQ(choices[0].role_id, op);
  EXPECT_FALSE(choices[0].held);
  EXPECT_EQ(choices[1].role_id, admin);
  EXPECT_TRUE(choices[1].held);
  EXPECT_EQ(choices[2].name, u"Shift A");
  EXPECT_TRUE(choices[2].held);
}

}  // namespace
