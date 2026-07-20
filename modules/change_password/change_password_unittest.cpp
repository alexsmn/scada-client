#include "modules/change_password/change_password.h"

#include "base/test/test_executor.h"
#include "events/local_events.h"
#include "model/security_node_ids.h"
#include "modules/change_password/change_password_dialog.h"
#include "node_service/node_model_mock.h"
#include "node_service/test/model_node_service.h"
#include "profile/profile.h"
#include "scada/client.h"
#include "scada/method_service_mock.h"
#include "scada/standard_node_ids.h"

#include <gmock/gmock.h>

using namespace testing;

namespace {

constexpr scada::NumericId kUserNodeId = 5001;

NodeRef MakeUserNode(ModelNodeService& node_service, scada::node scada_node) {
  auto node_model = std::make_shared<NiceMock<MockNodeModel>>();
  ON_CALL(*node_model, GetAttribute(scada::AttributeId::NodeId))
      .WillByDefault(Return(scada::NodeId{kUserNodeId, 1}));
  ON_CALL(*node_model, GetAttribute(scada::AttributeId::DisplayName))
      .WillByDefault(Return(scada::LocalizedText{u"Operator"}));
  ON_CALL(*node_model, GetScadaNode()).WillByDefault(Return(scada_node));
  return node_service.Add(scada::NodeId{kUserNodeId, 1}, std::move(node_model));
}

}  // namespace

class ChangePasswordTest : public Test {
 protected:
  ChangePasswordTest()
      : scada_client_{scada::services{.method_service = &method_service_}},
        user_node_{MakeUserNode(node_service_,
                                scada_client_.node({kUserNodeId, 1}))} {}

  void PollExecutor() { executor_.Poll(); }

  TestExecutor executor_;
  StrictMock<scada::MockMethodService> method_service_;
  scada::client scada_client_;
  ModelNodeService node_service_;
  NodeRef user_node_;
  LocalEvents local_events_;
  Profile profile_;
};

TEST_F(ChangePasswordTest, ReportsSuccessAfterMethodCallCompletes) {
  EXPECT_CALL(method_service_,
              Call(scada::NodeId{kUserNodeId, 1},
                   scada::security::id::UserType_ChangePassword, SizeIs(2), _))
      .WillOnce(Invoke([](auto, auto, auto, auto) {
        return scada::MakeMethodCallResult(scada::StatusCode::Good);
      }));

  ChangePassword(
      ChangePasswordContext{user_node_, executor_, local_events_, profile_},
      u"old", u"new");
  PollExecutor();

  ASSERT_EQ(local_events_.events().size(), 1);
  const auto& event = *local_events_.events().front();
  EXPECT_EQ(event.severity, scada::kSeverityNormal);
  EXPECT_NE(event.message.find(u"Changing password for user Operator"),
            std::u16string::npos);
}

TEST_F(ChangePasswordTest, ReportsFailureAfterMethodCallCompletes) {
  EXPECT_CALL(method_service_,
              Call(scada::NodeId{kUserNodeId, 1},
                   scada::security::id::UserType_ChangePassword, SizeIs(2), _))
      .WillOnce(Invoke([](auto, auto, auto, auto) {
        return scada::MakeMethodCallResult(
            scada::StatusCode::Bad_WrongMethodId);
      }));

  ChangePassword(
      ChangePasswordContext{user_node_, executor_, local_events_, profile_},
      u"old", u"new");
  PollExecutor();

  ASSERT_EQ(local_events_.events().size(), 1);
  const auto& event = *local_events_.events().front();
  EXPECT_EQ(event.severity, scada::kSeverityCritical);
  EXPECT_NE(event.message.find(u"Changing password for user Operator"),
            std::u16string::npos);
}
