#include "user_access/role_membership.h"

#include "base/awaitable.h"
#include "common/node_state.h"
#include "model/security_node_ids.h"
#include "node_service/node_ref.h"
#include "node_service/test/fake_node_service.h"
#include "scada/authorization.h"
#include "scada/node_id.h"
#include "scada/standard_node_ids.h"

#include <gtest/gtest.h>

#include <boost/asio/io_context.hpp>

namespace {

const scada::NodeId kRoleSet{scada::id::Server_ServerCapabilities_RoleSet,
                             scada::NamespaceIndexes::NS0};

class RoleMembershipTest : public ::testing::Test {
 protected:
  RoleMembershipTest() {
    node_service_.Add(scada::NodeState{
        .node_id = scada::security::id::IdentityMappingRuleType,
        .node_class = scada::NodeClass::ObjectType});
  }

  void AddRoleSet() {
    node_service_.Add(scada::NodeState{.node_id = kRoleSet,
                                       .node_class = scada::NodeClass::Object});
  }

  void AddRole(const scada::NodeId& role_id, std::u16string name) {
    node_service_.Add(
        scada::NodeState{.node_id = role_id,
                         .node_class = scada::NodeClass::Object,
                         .parent_id = kRoleSet,
                         .reference_type_id = scada::id::Organizes,
                         .attributes = {.display_name = std::move(name)}});
  }

  void AddRule(const scada::NodeId& role_id,
               scada::UInt32 rule_id,
               const std::string& criteria,
               scada::IdentityCriteriaType criteria_type =
                   scada::IdentityCriteriaType::kUserName) {
    node_service_.Add(scada::NodeState{
        .node_id = scada::NodeId{rule_id, 31},
        .node_class = scada::NodeClass::Object,
        .type_definition_id = scada::security::id::IdentityMappingRuleType,
        .parent_id = role_id,
        .reference_type_id = scada::id::Organizes,
        .properties = {
            {scada::security::id::IdentityMappingRuleType_CriteriaType,
             static_cast<scada::Int32>(criteria_type)},
            {scada::security::id::IdentityMappingRuleType_Criteria,
             criteria}}});
  }

  std::optional<std::vector<RoleMembership>> Read() {
    return RunAwaitable(
        io_, [this]() -> Awaitable<std::optional<std::vector<RoleMembership>>> {
          co_return co_await ReadRoleMemberships(io_.get_executor(),
                                                 node_service_);
        });
  }

  boost::asio::io_context io_;
  FakeNodeService node_service_;
};

TEST_F(RoleMembershipTest, ReadsEachRoleAndItsMembers) {
  AddRoleSet();
  const scada::NodeId op =
      scada::WellKnownRoleId(scada::WellKnownRole::kOperator);
  AddRole(op, u"Operator");
  AddRule(op, 1, "ivanov");
  AddRule(op, 2, "petrov");

  const auto roles = Read();

  ASSERT_TRUE(roles.has_value());
  ASSERT_EQ(roles->size(), 1u);
  EXPECT_EQ((*roles)[0].name, u"Operator");
  EXPECT_EQ((*roles)[0].members,
            (std::vector<std::u16string>{u"ivanov", u"petrov"}));
}

// A Role nobody holds is a real state — an empty Role is how one starts.
TEST_F(RoleMembershipTest, ARoleWithNoRulesHasNoMembers) {
  AddRoleSet();
  AddRole(scada::WellKnownRoleId(scada::WellKnownRole::kEngineer), u"Engineer");

  const auto roles = Read();

  ASSERT_TRUE(roles.has_value());
  ASSERT_EQ(roles->size(), 1u);
  EXPECT_TRUE((*roles)[0].members.empty());
}

// The well-known Roles' NODES cannot be deleted, which is the one structural
// fact the view reports about a Role.
TEST_F(RoleMembershipTest, MarksWellKnownRoles) {
  AddRoleSet();
  AddRole(scada::WellKnownRoleId(scada::WellKnownRole::kOperator), u"Operator");
  AddRole(scada::NodeId{7, scada::NamespaceIndexes::ROLE}, u"Shift A");

  const auto roles = Read();

  ASSERT_TRUE(roles.has_value());
  ASSERT_EQ(roles->size(), 2u);
  EXPECT_TRUE((*roles)[0].well_known);
  EXPECT_FALSE((*roles)[1].well_known);
}

// Only UserName criteria name an account; the other IdentityCriteriaType
// values describe token classes a username/password account never presents.
TEST_F(RoleMembershipTest, IgnoresNonUserNameCriteria) {
  AddRoleSet();
  const scada::NodeId op =
      scada::WellKnownRoleId(scada::WellKnownRole::kOperator);
  AddRole(op, u"Operator");
  AddRule(op, 1, "ivanov", scada::IdentityCriteriaType::kThumbprint);

  const auto roles = Read();

  ASSERT_TRUE(roles.has_value());
  ASSERT_EQ(roles->size(), 1u);
  EXPECT_TRUE((*roles)[0].members.empty());
}

// A conformant server always publishes the well-known Roles, so an empty
// browse is a failed read, not a server without Roles.
TEST_F(RoleMembershipTest, AbsentRoleSetIsUnknownNotEmpty) {
  EXPECT_FALSE(Read().has_value());
}

TEST_F(RoleMembershipTest, EmptyRoleSetIsUnknownNotEmpty) {
  AddRoleSet();

  EXPECT_FALSE(Read().has_value());
}

// The per-user inversion both the Users grid and the RBAC inspector consume.
// Each held Role carries its ID as well as its name: the name is what an
// operator reads, the id is what the effective permissions are derived from.
TEST_F(RoleMembershipTest, InvertsMembershipByAccount) {
  const scada::NodeId op =
      scada::WellKnownRoleId(scada::WellKnownRole::kOperator);
  const scada::NodeId admin =
      scada::WellKnownRoleId(scada::WellKnownRole::kConfigureAdmin);
  const std::vector<RoleMembership> roles = {
      {.node_id = op, .name = u"Operator", .members = {u"ivanov", u"petrov"}},
      {.node_id = admin, .name = u"ConfigureAdmin", .members = {u"ivanov"}},
      {.node_id = scada::WellKnownRoleId(scada::WellKnownRole::kEngineer),
       .name = u"Engineer",
       .members = {}}};

  const auto by_account = RolesByAccount(roles);

  ASSERT_EQ(by_account.size(), 2u);
  const auto& held = by_account.at(u"ivanov");
  ASSERT_EQ(held.size(), 2u);
  EXPECT_EQ(held[0].node_id, op);
  EXPECT_EQ(held[0].name, u"Operator");
  EXPECT_EQ(held[1].node_id, admin);
  EXPECT_EQ(by_account.at(u"petrov").size(), 1u);
  // An account named by no rule is simply absent; the caller decides what that
  // means.
  EXPECT_FALSE(by_account.contains(u"audit"));
}

}  // namespace
