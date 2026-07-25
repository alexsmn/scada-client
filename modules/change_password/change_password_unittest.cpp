#include "modules/change_password/change_password.h"

#include "base/test/test_executor.h"
#include "common/node_state.h"
#include "events/local_events.h"
#include "model/security_node_ids.h"
#include "modules/change_password/change_password_dialog.h"
#include "node_service/test/fake_node_service.h"
#include "profile/profile.h"
#include "scada/client.h"
#include "scada/method_service_mock.h"
#include "scada/standard_node_ids.h"

#include <gmock/gmock.h>

using namespace testing;

namespace {

constexpr scada::NumericId kUserNodeId = 5001;

}  // namespace

class ChangePasswordTest : public Test {
 protected:
  ChangePasswordTest()
      : user_node_{node_service_.Add(scada::NodeState{
            .node_id = scada::NodeId{kUserNodeId, 1},
            .attributes = {.display_name =
                               scada::LocalizedText{u"Operator"}}})} {}

  void PollExecutor() { executor_.Poll(); }

  TestExecutor executor_;
  StrictMock<scada::MockMethodService> method_service_;
  // The fake backs GetScadaNode() with these services, so method calls made
  // through the node cursor land on |method_service_|. Declared after it so it
  // outlives nothing it depends on.
  FakeNodeService node_service_{
      scada::services{.method_service = &method_service_}};
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
  EXPECT_NE(event.message.text.find(u"Changing password for user Operator"),
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
  EXPECT_NE(event.message.text.find(u"Changing password for user Operator"),
            std::u16string::npos);
}
