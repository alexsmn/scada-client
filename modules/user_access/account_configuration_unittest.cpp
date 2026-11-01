#include "user_access/account_configuration.h"

#include "common/node_state.h"
#include "model/security_node_ids.h"
#include "node_service/test/fake_node_service.h"

#include <gtest/gtest.h>

namespace {

class AccountConfigurationTest : public ::testing::Test {
 protected:
  AccountConfigurationTest() {
    node_service_.Add(
        scada::NodeState{.node_id = scada::security::id::UserType,
                         .node_class = scada::NodeClass::ObjectType});
  }

  NodeRef AddUser(scada::NumericId id, scada::UserConfiguration mask) {
    return node_service_.Add(scada::NodeState{
        .node_id = scada::NodeId{id, 5},
        .node_class = scada::NodeClass::Object,
        .type_definition_id = scada::security::id::UserType,
        .properties = {{scada::security::id::UserType_UserConfiguration,
                        static_cast<scada::Int32>(mask)}}});
  }

  FakeNodeService node_service_;
};

// The server marks the inter-process service account NoDelete, and that flag
// — read off the account itself — is what keeps Delete off its menu (backlog
// 882).
TEST_F(AccountConfigurationTest, ReadsTheAccountsMask) {
  const NodeRef svc = AddUser(100, scada::UserConfiguration::kNoDelete);
  const NodeRef ivanov = AddUser(101, scada::UserConfiguration::kDisabled);

  EXPECT_EQ(ResidentUserConfiguration(svc),
            scada::UserConfiguration::kNoDelete);
  EXPECT_TRUE(IsUndeletableAccount(svc));
  EXPECT_EQ(ResidentUserConfiguration(ivanov),
            scada::UserConfiguration::kDisabled);
  EXPECT_FALSE(IsUndeletableAccount(ivanov));
}

// Anything that is not an account has no mask, and is never "undeletable" on
// this account's say-so.
TEST_F(AccountConfigurationTest, NonAccountsHaveNoMask) {
  const NodeRef folder = node_service_.Add(scada::NodeState{
      .node_id = scada::NodeId{29, 7}, .node_class = scada::NodeClass::Object});

  EXPECT_EQ(ResidentUserConfiguration(folder), std::nullopt);
  EXPECT_FALSE(IsUndeletableAccount(folder));
  EXPECT_FALSE(IsUndeletableAccount(NodeRef{}));
}

}  // namespace
