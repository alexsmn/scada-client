#include "user_access/users_grid.h"

#include "base/awaitable.h"
#include "common/node_state.h"
#include "model/security_node_ids.h"
#include "node_service/node_ref.h"
#include "node_service/test/fake_node_service.h"
#include "scada/authorization.h"
#include "scada/node_id.h"
#include "scada/standard_node_ids.h"
#include "scada/user_management_encoding.h"
#include "scada/variant.h"

#include <gtest/gtest.h>

#include <boost/asio/io_context.hpp>

#include <optional>

namespace {

const scada::NodeId kUsersProperty{scada::id::UserManagement_Users,
                                   scada::NamespaceIndexes::NS0};
const scada::NodeId kRoleSet{scada::id::Server_ServerCapabilities_RoleSet,
                             scada::NamespaceIndexes::NS0};

// The grid over the standard user model: one Read of UserManagement.Users,
// joined against the RoleSet membership rules.
class UsersGridTest : public ::testing::Test {
 protected:
  UsersGridTest() {
    node_service_.Add(scada::NodeState{
        .node_id = scada::security::id::IdentityMappingRuleType,
        .node_class = scada::NodeClass::ObjectType});
  }

  // Publishes the Users property. Omitting it entirely, or seeding a value the
  // decoder rejects, is the "could not read" case.
  void SetUsers(std::vector<scada::UserManagementDataType> users) {
    node_service_.Add(scada::NodeState{
        .node_id = kUsersProperty,
        .node_class = scada::NodeClass::Variable,
        .attributes = {.value = scada::EncodeUserManagementUsers(users)}});
  }

  void SetUnreadableUsers() {
    node_service_.Add(
        scada::NodeState{.node_id = kUsersProperty,
                         .node_class = scada::NodeClass::Variable,
                         .attributes = {.value = scada::Variant{42}}});
  }

  void AddRoleSet() {
    node_service_.Add(scada::NodeState{.node_id = kRoleSet,
                                       .node_class = scada::NodeClass::Object});
  }

  // Adds a Role under the RoleSet with one identity mapping rule naming
  // `member`.
  void AddRoleWithMember(scada::UInt32 role_id,
                         std::u16string role_name,
                         scada::UInt32 rule_id,
                         const std::string& member,
                         scada::IdentityCriteriaType criteria_type =
                             scada::IdentityCriteriaType::kUserName) {
    node_service_.Add(
        scada::NodeState{.node_id = scada::NodeId{role_id, 0},
                         .node_class = scada::NodeClass::Object,
                         .parent_id = kRoleSet,
                         .reference_type_id = scada::id::Organizes,
                         .attributes = {.display_name = role_name}});
    node_service_.Add(scada::NodeState{
        .node_id = scada::NodeId{rule_id, 31},
        .node_class = scada::NodeClass::Object,
        .type_definition_id = scada::security::id::IdentityMappingRuleType,
        .parent_id = scada::NodeId{role_id, 0},
        .reference_type_id = scada::id::Organizes,
        .properties = {
            {scada::security::id::IdentityMappingRuleType_CriteriaType,
             static_cast<scada::Int32>(criteria_type)},
            {scada::security::id::IdentityMappingRuleType_Criteria, member}}});
  }

  std::optional<std::vector<UserGridRow>> Build() {
    return RunAwaitable(
        io_, [this]() -> Awaitable<std::optional<std::vector<UserGridRow>>> {
          co_return co_await BuildUsersGrid(io_.get_executor(), node_service_);
        });
  }

  boost::asio::io_context io_;
  FakeNodeService node_service_;
};

// The whole account list arrives in one Read — no per-user Browse.
TEST_F(UsersGridTest, ReadsEveryAccountFromTheUsersProperty) {
  SetUsers({{.user_name = "root"},
            {.user_name = "ivanov", .description = "Dispatcher"}});
  AddRoleSet();
  AddRoleWithMember(15680, u"Operator", 1, "ivanov");

  const auto rows = Build();

  ASSERT_TRUE(rows.has_value());
  ASSERT_EQ(rows->size(), 2u);
  EXPECT_EQ((*rows)[0].name, u"root");
  EXPECT_EQ((*rows)[1].name, u"ivanov");
  EXPECT_EQ((*rows)[1].description, u"Dispatcher");
}

// The Roles column IS the authorization model, so a membership rule is what
// puts a Role on a row — not the retired access-rights bitmask.
TEST_F(UsersGridTest, JoinsRoleMembershipFromTheRoleSet) {
  SetUsers({{.user_name = "ivanov"}, {.user_name = "audit"}});
  AddRoleSet();
  AddRoleWithMember(15680, u"Operator", 1, "ivanov");
  AddRoleWithMember(15716, u"ConfigureAdmin", 2, "ivanov");

  const auto rows = Build();

  ASSERT_TRUE(rows.has_value());
  ASSERT_EQ(rows->size(), 2u);
  ASSERT_TRUE((*rows)[0].roles.has_value());
  ASSERT_EQ((*rows)[0].roles->size(), 2u);
  // The id travels with the name: the inspector derives permissions from it.
  EXPECT_EQ((*(*rows)[0].roles)[0].node_id, (scada::NodeId{15680, 0}));
  EXPECT_EQ((*(*rows)[0].roles)[0].name, u"Operator");
  EXPECT_EQ((*(*rows)[0].roles)[1].name, u"ConfigureAdmin");
  // An account named by no rule holds no Role — a REAL answer, and an empty
  // vector rather than nullopt.
  ASSERT_TRUE((*rows)[1].roles.has_value());
  EXPECT_TRUE((*rows)[1].roles->empty());
}

// Only UserName criteria name an account; the other IdentityCriteriaType
// values describe token classes a username/password account never presents,
// so matching on them would invent a membership.
TEST_F(UsersGridTest, IgnoresNonUserNameCriteria) {
  SetUsers({{.user_name = "ivanov"}});
  AddRoleSet();
  AddRoleWithMember(15680, u"Operator", 1, "ivanov",
                    scada::IdentityCriteriaType::kThumbprint);

  const auto rows = Build();

  ASSERT_TRUE(rows.has_value());
  ASSERT_EQ(rows->size(), 1u);
  ASSERT_TRUE((*rows)[0].roles.has_value());
  EXPECT_TRUE((*rows)[0].roles->empty());
}

// A RoleSet that could not be read must not render as "holds nothing" — that
// would state a fact the client does not have.
TEST_F(UsersGridTest, UnreadableRoleSetIsNotNoRoles) {
  SetUsers({{.user_name = "ivanov"}});
  // No RoleSet node at all.

  const auto rows = Build();

  ASSERT_TRUE(rows.has_value());
  ASSERT_EQ(rows->size(), 1u);
  EXPECT_FALSE((*rows)[0].roles.has_value());
}

// The Status column comes from the Part 18 UserConfigurationMask.
TEST_F(UsersGridTest, ReportsTheDisabledFlag) {
  SetUsers({{.user_name = "ivanov",
             .user_configuration = scada::UserConfiguration::kDisabled},
            {.user_name = "petrov"}});
  AddRoleSet();

  const auto rows = Build();

  ASSERT_TRUE(rows.has_value());
  ASSERT_EQ(rows->size(), 2u);
  EXPECT_STREQ(UserStatusLabelKey((*rows)[0].user_configuration), "Disabled");
  EXPECT_STREQ(UserStatusLabelKey((*rows)[1].user_configuration), "Enabled");
  // Another bit set is not the disable bit.
  EXPECT_STREQ(UserStatusLabelKey(scada::UserConfiguration::kNoDelete),
               "Enabled");
}

// An account list that could not be read is nullopt, never an empty grid: a
// non-administrator gets Bad_UserAccessDenied here by design (Part 18 §5.2.1),
// and "you may not see the users" must not look like "there are no users".
TEST_F(UsersGridTest, UnreadableUsersPropertyYieldsNullopt) {
  SetUnreadableUsers();
  AddRoleSet();

  EXPECT_FALSE(Build().has_value());
}

TEST_F(UsersGridTest, AbsentUsersPropertyYieldsNullopt) {
  AddRoleSet();

  EXPECT_FALSE(Build().has_value());
}

// A server with no accounts is an empty list, which is a real answer and
// distinct from the nullopt above.
TEST_F(UsersGridTest, NoAccountsIsAnEmptyList) {
  SetUsers({});
  AddRoleSet();

  const auto rows = Build();

  ASSERT_TRUE(rows.has_value());
  EXPECT_TRUE(rows->empty());
}

}  // namespace
