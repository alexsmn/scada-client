#include "components/change_password/change_password.h"

#include "base/test/test_executor.h"
#include "components/change_password/change_password_dialog.h"
#include "events/local_events.h"
#include "model/security_node_ids.h"
#include "node_service/node_model_mock.h"
#include "profile/profile.h"
#include "scada/client.h"
#include "scada/method_service_mock.h"
#include "scada/standard_node_ids.h"

#include <gmock/gmock.h>

using namespace testing;

namespace {

constexpr scada::NumericId kUserNodeId = 5001;

NodeRef MakeUserNode(scada::node scada_node) {
  auto node_model = std::make_shared<NiceMock<MockNodeModel>>();
  ON_CALL(*node_model, GetAttribute(scada::AttributeId::NodeId))
      .WillByDefault(Return(scada::NodeId{kUserNodeId, 1}));
  ON_CALL(*node_model, GetAttribute(scada::AttributeId::DisplayName))
      .WillByDefault(Return(scada::LocalizedText{u"Operator"}));
  ON_CALL(*node_model, GetScadaNode()).WillByDefault(Return(scada_node));
  return node_model;
}

}  // namespace

class ChangePasswordTest : public Test {
 protected:
  ChangePasswordTest()
      : scada_client_{scada::services{.method_service = &method_service_}},
        user_node_{MakeUserNode(scada_client_.node({kUserNodeId, 1}))} {}

  void PollExecutor() { executor_->Poll(); }

  std::shared_ptr<TestExecutor> executor_ = std::make_shared<TestExecutor>();
  StrictMock<scada::MockMethodService> method_service_;
  scada::client scada_client_;
  NodeRef user_node_;
  LocalEvents local_events_;
  Profile profile_;
};

TEST_F(ChangePasswordTest, ReportsSuccessAfterMethodCallCompletes) {
  scada::StatusCallback callback;
  EXPECT_CALL(method_service_,
              Call(scada::NodeId{kUserNodeId, 1},
                   security::id::UserType_ChangePassword, SizeIs(2), _, _))
      .WillOnce(SaveArg<4>(&callback));

  ChangePassword(ChangePasswordContext{user_node_, executor_, local_events_,
                                       profile_},
                 u"old", u"new");
  PollExecutor();
  ASSERT_TRUE(static_cast<bool>(callback));
  EXPECT_TRUE(local_events_.events().empty());

  callback(scada::StatusCode::Good);
  PollExecutor();

  ASSERT_EQ(local_events_.events().size(), 1);
  const auto& event = *local_events_.events().front();
  EXPECT_EQ(event.severity, scada::kSeverityNormal);
  EXPECT_NE(event.message.find(u"Changing password for user Operator"),
            std::u16string::npos);
}

TEST_F(ChangePasswordTest, ReportsFailureAfterMethodCallCompletes) {
  scada::StatusCallback callback;
  EXPECT_CALL(method_service_,
              Call(scada::NodeId{kUserNodeId, 1},
                   security::id::UserType_ChangePassword, SizeIs(2), _, _))
      .WillOnce(SaveArg<4>(&callback));

  ChangePassword(ChangePasswordContext{user_node_, executor_, local_events_,
                                       profile_},
                 u"old", u"new");
  PollExecutor();
  ASSERT_TRUE(static_cast<bool>(callback));

  callback(scada::StatusCode::Bad_WrongMethodId);
  PollExecutor();

  ASSERT_EQ(local_events_.events().size(), 1);
  const auto& event = *local_events_.events().front();
  EXPECT_EQ(event.severity, scada::kSeverityCritical);
  EXPECT_NE(event.message.find(u"Changing password for user Operator"),
            std::u16string::npos);
}
