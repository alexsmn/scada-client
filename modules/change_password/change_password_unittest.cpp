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

// Both standard methods live on the UserManagement object, not on the per-user
// node: the standard user model keys accounts by NAME and has no node to call
// a method on (OPC UA Part 18 §5.2).
const scada::NodeId kUserManagement{
    scada::id::Server_ServerConfiguration_UserManagement,
    scada::NamespaceIndexes::NS0};
const scada::NodeId kChangePasswordMethod{
    scada::id::UserManagement_ChangePassword, scada::NamespaceIndexes::NS0};
const scada::NodeId kModifyUserMethod{scada::id::UserManagement_ModifyUser,
                                      scada::NamespaceIndexes::NS0};

}  // namespace

class ChangePasswordTest : public Test {
 protected:
  ChangePasswordTest()
      : user_node_{node_service_.Add(scada::NodeState{
            .node_id = scada::NodeId{kUserNodeId, 1},
            .attributes = {.display_name =
                               scada::LocalizedText{u"Operator"}}})} {
    node_service_.Add(scada::NodeState{.node_id = kUserManagement});
  }

  ChangePasswordContext Context(bool self_service) {
    return ChangePasswordContext{.user_ = user_node_,
                                 .executor_ = executor_,
                                 .local_events_ = local_events_,
                                 .profile_ = profile_,
                                 .node_service_ = &node_service_,
                                 .self_service_ = self_service};
  }

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

// Changing one's OWN password is Part 18 §5.2.8: it proves the old password,
// so both arguments go across.
TEST_F(ChangePasswordTest, SelfServiceCallsTheStandardChangePassword) {
  EXPECT_CALL(method_service_,
              Call(kUserManagement, kChangePasswordMethod, SizeIs(2), _))
      .WillOnce(Invoke([](auto, auto, auto, auto) {
        return scada::MakeMethodCallResult(scada::StatusCode::Good);
      }));

  ChangePassword(Context(/*self_service=*/true), u"old", u"new");
  PollExecutor();

  ASSERT_EQ(local_events_.events().size(), 1);
  const auto& event = *local_events_.events().front();
  EXPECT_EQ(event.severity, scada::kSeverityNormal);
  EXPECT_NE(event.message.text.find(u"Changing password for user Operator"),
            std::u16string::npos);
}

// An ADMINISTRATOR resetting someone else's account is §5.2.7 ModifyUser,
// which does not require the old password — so it must not be sent, and the
// account is named rather than pointed at by node. Seven arguments per the
// spec's signature.
TEST_F(ChangePasswordTest, AdminResetCallsModifyUserAndSendsNoOldPassword) {
  std::vector<scada::Variant> sent;
  EXPECT_CALL(method_service_,
              Call(kUserManagement, kModifyUserMethod, SizeIs(7), _))
      .WillOnce(Invoke([&sent](auto, auto, auto arguments, auto) {
        sent = arguments;
        return scada::MakeMethodCallResult(scada::StatusCode::Good);
      }));

  ChangePassword(Context(/*self_service=*/false), u"old", u"new");
  PollExecutor();

  ASSERT_EQ(sent.size(), 7u);
  // The account is identified by name, and the old password appears nowhere.
  EXPECT_EQ(sent[0].get_or(scada::LocalizedText{}), scada::LocalizedText{u"Operator"});
  EXPECT_TRUE(sent[1].get_or(false));
  EXPECT_EQ(sent[2].get_or(scada::LocalizedText{}), scada::LocalizedText{u"new"});
  for (const scada::Variant& argument : sent) {
    EXPECT_NE(argument.get_or(scada::LocalizedText{}),
              scada::LocalizedText{u"old"});
  }
}

TEST_F(ChangePasswordTest, ReportsFailureAfterMethodCallCompletes) {
  EXPECT_CALL(method_service_,
              Call(kUserManagement, kChangePasswordMethod, SizeIs(2), _))
      .WillOnce(Invoke([](auto, auto, auto, auto) {
        return scada::MakeMethodCallResult(
            scada::StatusCode::Bad_WrongMethodId);
      }));

  ChangePassword(Context(/*self_service=*/true), u"old", u"new");
  PollExecutor();

  ASSERT_EQ(local_events_.events().size(), 1);
  const auto& event = *local_events_.events().front();
  EXPECT_EQ(event.severity, scada::kSeverityCritical);
  EXPECT_NE(event.message.text.find(u"Changing password for user Operator"),
            std::u16string::npos);
}
